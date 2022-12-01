/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the <CE Title> Example
*              for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2022, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include <inttypes.h>
#include <stdio.h>

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "cli_task.h"
#include "resource_map.h"
#include "xensiv_bgt60trxx_mtb.h"
#include "xensiv_radar_presence.h"

#define XENSIV_BGT60TRXX_CONF_IMPL
#include "radar_settings.h"

/*******************************************************************************
* Macros
********************************************************************************/

#define XENSIV_BGT60TRXX_SPI_FREQUENCY      (25000000UL)

#define NUM_SAMPLES_PER_FRAME               (XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP *\
                                             XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME *\
                                             XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS)

#define NUM_CHIRPS_PER_FRAME                XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME
#define NUM_SAMPLES_PER_CHIRP               XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP

/* RTOS tasks */
#define MAIN_TASK_NAME                      "main_task"
#define MAIN_TASK_STACK_SIZE                (configMINIMAL_STACK_SIZE * 2)
#define MAIN_TASK_PRIORITY                  (configMAX_PRIORITIES - 1)
#define PROCESSING_TASK_NAME                "processing_task"
#define PROCESSING_TASK_STACK_SIZE          (configMINIMAL_STACK_SIZE * 2)
#define PROCESSING_TASK_PRIORITY            (configMAX_PRIORITIES - 2)
#define CLI_TASK_NAME                       "cli_task"
#define CLI_TASK_STACK_SIZE                 (configMINIMAL_STACK_SIZE * 10)
#define CLI_TASK_PRIORITY                   (tskIDLE_PRIORITY)

/* Interrupt priorities */
#define GPIO_INTERRUPT_PRIORITY             (6)


/*******************************************************************************
* Function Prototypes
********************************************************************************/
static void main_task(void *pvParameters);
static void processing_task(void *pvParameters);
static void timer_callbak(TimerHandle_t xTimer);

static int32_t init_leds(void);
static int32_t init_sensor(void);
static void xensiv_bgt60trxx_interrupt_handler(void* args, cyhal_gpio_event_t event);
void presence_detection_cb(xensiv_radar_presence_handle_t handle,
                           const xensiv_radar_presence_event_t* event,
                           void *data);


/*******************************************************************************
* Global Variables
********************************************************************************/
static cyhal_spi_t spi_obj;
static xensiv_bgt60trxx_mtb_t bgt60_obj;
static uint16_t bgt60_buffer[NUM_SAMPLES_PER_FRAME] __attribute__((aligned(2)));
static float32_t frame[NUM_SAMPLES_PER_FRAME];
static float32_t avg_chirp[NUM_SAMPLES_PER_CHIRP];

static TaskHandle_t main_task_handler;
static TaskHandle_t processing_task_handler;
static TimerHandle_t timer_handler;

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function for CM4 CPU. It initializes BSP, creates FreeRTOS  
* main task and starts the scheduler.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

#ifdef TARGET_CYSBSYSKIT_DEV_01

    /* Initialize the User LED */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

#endif

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "Radar presence"
           "****************** \r\n\n"
           "Human presence detection using XENSIV 60-GHz radar\r\n"
           );

    printf("Press ENTER to enter setup mode, press ESC to quit setup mode \r\n");
       

    /* Create the RTOS task */
    if (xTaskCreate(main_task, MAIN_TASK_NAME, MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &main_task_handler) != pdPASS)
    {
        CY_ASSERT(0);
    }

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    CY_ASSERT(0);
}

/*******************************************************************************
* Function Name: main_task
********************************************************************************
* Summary:
* This is the main task.
*    1. Creates a timer to toggle user LED
*    2. Create the processing RTOS task
*    3. Initializes the hardware interface to the sensor and LEDs
*    4. Initializes the radar device
*    5. In an infinite loop
*       - Waits for interrupt from radar device indicating availability of data
*       - Reads the data, converts it to floating point and notifies the processing task
* Parameters:
*  void
*
* Return:
*  none
*
*******************************************************************************/
static __NO_RETURN void main_task(void *pvParameters)
{
    (void)pvParameters;

    timer_handler = xTimerCreate("timer", pdMS_TO_TICKS(1000), pdTRUE, NULL, timer_callbak);    
    if (timer_handler == NULL)
    {
        CY_ASSERT(0);
    }

    if (xTimerStart(timer_handler, 0) != pdPASS)
    {
        CY_ASSERT(0);
    }

    if (xTaskCreate(processing_task, PROCESSING_TASK_NAME, PROCESSING_TASK_STACK_SIZE, NULL, PROCESSING_TASK_PRIORITY, &processing_task_handler) != pdPASS)
    {
        CY_ASSERT(0);
    }

    if (init_sensor() != 0)
    {
        CY_ASSERT(0);
    }

    if (init_leds () != 0)
    {
        CY_ASSERT(0);
    }


    if (xensiv_bgt60trxx_start_frame(&bgt60_obj.dev, true) != XENSIV_BGT60TRXX_STATUS_OK)
    {
        CY_ASSERT(0);
    }

    for(;;)
    {
        /* Wait for the GPIO interrupt to indicate that another slice is available */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (xensiv_bgt60trxx_get_fifo_data(&bgt60_obj.dev, 
                                           bgt60_buffer,
                                           NUM_SAMPLES_PER_FRAME) == XENSIV_BGT60TRXX_STATUS_OK)
        {
            /* Data preprocessing */
            uint16_t *bgt60_buffer_ptr = &bgt60_buffer[0];
            float32_t *frame_ptr = &frame[0];
            for (int32_t sample = 0; sample < NUM_SAMPLES_PER_FRAME; ++sample)
            {
                *frame_ptr++ = ((float32_t)(*bgt60_buffer_ptr++) / 4096.0F);
            }

            // calculate the average of the chirps first
            arm_fill_f32(0, avg_chirp, NUM_SAMPLES_PER_CHIRP);

            for (int chirp = 0; chirp < NUM_CHIRPS_PER_FRAME; chirp++)
            {
                arm_add_f32(avg_chirp, &frame[NUM_SAMPLES_PER_CHIRP * chirp], avg_chirp, NUM_SAMPLES_PER_CHIRP);
            }

            arm_scale_f32(avg_chirp, 1.0f / NUM_CHIRPS_PER_FRAME, avg_chirp, NUM_SAMPLES_PER_CHIRP);


            /* Tell processing task to take over */
            xTaskNotifyGive(processing_task_handler);
        }
    }
}


/*******************************************************************************
* Function Name: processing_task
********************************************************************************
* Summary:
* This is the data processing task.
*    1. Initializes the presence sensing library and register an event callback
*    2. It creates a console task to handle parameter configuration for the library
*    3. In a loop
*       - receives notification from main task
*       - executes the presence algorithm and provides the result on terminal and LEDs
*
* Parameters:
*  void
*
* Return:
*  None
*
*******************************************************************************/
static __NO_RETURN void processing_task(void *pvParameters)
{
    (void)pvParameters;

    static const xensiv_radar_presence_config_t default_config =
    {
        .bandwidth                         = 460E6,
        .num_samples_per_chirp             = XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP,
        .micro_fft_decimation_enabled      = false,
        .micro_fft_size                    = 128,
        .macro_threshold                   = 0.5f,
        .micro_threshold                   = 12.5f,
        .min_range_bin                     = 1,
        .max_range_bin                     = 5,
        .macro_compare_interval_ms         = 250,
        .macro_movement_validity_ms        = 1000,
        .micro_movement_validity_ms        = 4000,
        .macro_movement_confirmations      = 0,
        .macro_trigger_range               = 1,
        .mode                              = XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO,
        .macro_fft_bandpass_filter_enabled = false,
        .micro_movement_compare_idx       = 5
    };

    xensiv_radar_presence_handle_t handle;

    xensiv_radar_presence_set_malloc_free(pvPortMalloc,
                                          vPortFree);

    if (xensiv_radar_presence_alloc(&handle, &default_config) != 0)
    {
        CY_ASSERT(0);
    }

    xensiv_radar_presence_set_callback(handle, presence_detection_cb, NULL);

    if (xTaskCreate(console_task, CLI_TASK_NAME, CLI_TASK_STACK_SIZE, handle, CLI_TASK_PRIORITY, NULL) != pdPASS)
    {
        CY_ASSERT(0);
    }

    for(;;)
    {
        /* Wait for frame data available to process */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        xensiv_radar_presence_process_frame(handle, frame, xTaskGetTickCount() * portTICK_PERIOD_MS);
    }
}


/*******************************************************************************
* Function Name: presence_detection_cb
********************************************************************************
* Summary:
* This is the callback function o indicate presence/absence events on terminal 
* and LEDs.
* Parameters:
*  void
*
* Return:
*  None
*
*******************************************************************************/
void presence_detection_cb(xensiv_radar_presence_handle_t handle,
                           const xensiv_radar_presence_event_t* event,
                           void *data)
{
    (void)handle;
    (void)data;

    switch (event->state)
    {
        case XENSIV_RADAR_PRESENCE_STATE_MACRO_PRESENCE:
            cyhal_gpio_write(LED_RGB_RED, true);
            cyhal_gpio_write(LED_RGB_GREEN, false);
            printf("[INFO] macro presence %" PRIi32 " %" PRIi32 "\n",
                   event->range_bin,
                   event->timestamp);
            break;

        case XENSIV_RADAR_PRESENCE_STATE_MICRO_PRESENCE:
            cyhal_gpio_write(LED_RGB_RED, true);
            cyhal_gpio_write(LED_RGB_GREEN, false);
            printf("[INFO] micro presence %" PRIi32 " %" PRIi32 "\n",
                   event->range_bin,
                   event->timestamp);
            break;

        case XENSIV_RADAR_PRESENCE_STATE_ABSENCE:
            printf("[INFO] absence %" PRIu32 "\n", event->timestamp);
            cyhal_gpio_write(LED_RGB_RED, false);
            cyhal_gpio_write(LED_RGB_GREEN, true);
            break;

        default:
            printf("[WARN]: Unknown reported state in event handling\n");
            break;
    }
}


/*******************************************************************************
* Function Name: init_sensor
********************************************************************************
* Summary:
* This function configures the SPI interface, initializes radar and interrupt
* service routine to indicate the availability of radar data. 
* 
* Parameters:
*  void
*
* Return:
*  Success or error 
*
*******************************************************************************/
static int32_t init_sensor(void)
{
    if (cyhal_spi_init(&spi_obj,
                       PIN_XENSIV_BGT60TRXX_SPI_MOSI,
                       PIN_XENSIV_BGT60TRXX_SPI_MISO,
                       PIN_XENSIV_BGT60TRXX_SPI_SCLK,
                       NC,
                       NULL,
                       8,
                       CYHAL_SPI_MODE_00_MSB,
                       false) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: cyhal_spi_init failed\n");
        return -1;
    }

    /* Reduce drive strength to improve EMI */
    Cy_GPIO_SetSlewRate(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CY_GPIO_SLEW_FAST);
    Cy_GPIO_SetDriveSel(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CY_GPIO_DRIVE_1_8);
    Cy_GPIO_SetSlewRate(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CY_GPIO_SLEW_FAST);
    Cy_GPIO_SetDriveSel(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CY_GPIO_DRIVE_1_8);

    /* Set the data rate to 25 Mbps */
    if (cyhal_spi_set_frequency(&spi_obj, XENSIV_BGT60TRXX_SPI_FREQUENCY) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: cyhal_spi_set_frequency failed\n");
        return -1;
    }

    /* Enable LDO */
    if (cyhal_gpio_init(PIN_XENSIV_BGT60TRXX_LDO_EN,
                        CYHAL_GPIO_DIR_OUTPUT,
                        CYHAL_GPIO_DRIVE_STRONG,
                        true) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: LDO_EN cyhal_gpio_init failed\n");
        return -1;
    }

    /* Wait LDO stable */
    (void)cyhal_system_delay_ms(5);

    if (xensiv_bgt60trxx_mtb_init(&bgt60_obj, 
                                  &spi_obj, 
                                  PIN_XENSIV_BGT60TRXX_SPI_CSN, 
                                  PIN_XENSIV_BGT60TRXX_RSTN, 
                                  register_list, 
                                  XENSIV_BGT60TRXX_CONF_NUM_REGS) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: xensiv_bgt60trxx_mtb_init failed\n");
        return -1;
    }

    if (xensiv_bgt60trxx_mtb_interrupt_init(&bgt60_obj,
                                            NUM_SAMPLES_PER_FRAME,
                                            PIN_XENSIV_BGT60TRXX_IRQ,
                                            GPIO_INTERRUPT_PRIORITY,
                                            xensiv_bgt60trxx_interrupt_handler,
                                            NULL) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: xensiv_bgt60trxx_mtb_interrupt_init failed\n");
        return -1;
    }

    return 0;
}


/*******************************************************************************
* Function Name: xensiv_bgt60trxx_interrupt_handler
********************************************************************************
* Summary:
* This is the interrupt handler to react on sensor indicating the availability 
* of new data
*    1. Notifies main task on interrupt from sensor
*
* Parameters:
*  void
*
* Return:
*  none
*
*******************************************************************************/
#if defined(CYHAL_API_VERSION) && (CYHAL_API_VERSION >= 2)
static void xensiv_bgt60trxx_interrupt_handler(void *args, cyhal_gpio_event_t event)
#else
static void xensiv_bgt60trxx_interrupt_handler(void *args, cyhal_gpio_irq_event_t event)
#endif
{
    CY_UNUSED_PARAMETER(args);
    CY_UNUSED_PARAMETER(event);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(main_task_handler, &xHigherPriorityTaskWoken);

    /* Context switch needed? */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/*******************************************************************************
* Function Name: init_leds
********************************************************************************
* Summary:
* This function initializes the GPIOs for LEDs and set them to off state.
* Parameters:
*  void
*
* Return:
*  Success or error
*
*******************************************************************************/
static int32_t init_leds(void)
{

    if(cyhal_gpio_init(LED_RGB_RED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false)!= CY_RSLT_SUCCESS)
    {
        printf("ERROR: GPIO initialization for LED_RGB_RED failed\n");
        return -1;
    }

    if( cyhal_gpio_init(LED_RGB_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false)!= CY_RSLT_SUCCESS)
    {
        printf("ERROR: GPIO initialization for LED_RGB_GREEN failed\n");
        return -1;
    }
    
    if( cyhal_gpio_init(LED_RGB_BLUE, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false)!= CY_RSLT_SUCCESS)
    {
        printf("ERROR: GPIO initialization for LED_RGB_BLUE failed\n");
        return -1;
    }

    return 0;
}


/*******************************************************************************
* Function Name: timer_callbak
********************************************************************************
* Summary:
* This is the timer_callback which toggles the LED
*
* Parameters:
*  void
*
* Return:
*  none
*
*******************************************************************************/
static void timer_callbak(TimerHandle_t xTimer)
{
    (void)xTimer;

#ifdef TARGET_CYSBSYSKIT_DEV_01
    cyhal_gpio_toggle(CYBSP_USER_LED);
#endif
}

/* [] END OF FILE */
