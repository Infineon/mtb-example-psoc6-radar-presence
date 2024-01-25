/* ***************************************************************************
** File name: main.c
**
** Description: This is the main file for PSOC6 Radar Presence Code Example.
**
** ===========================================================================
** Copyright (C) 2023 Infineon Technologies AG. All rights reserved.
** ===========================================================================
**
** ===========================================================================
** Infineon Technologies AG (INFINEON) is supplying this file for use
** exclusively with Infineon's sensor products. This file can be freely
** distributed within development tools and software supporting such
** products.
**
** THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
** OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
** INFINEON SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON
** WHATSOEVER.
** ===========================================================================
*/

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
#include "radar_config_optimizer.h"

#include "radar_low_framerate_config.h"

#include "radar_high_framerate_config.h"

#define XENSIV_RADAR_PRESENCE_SETTINGS_H_IMPL
#include "presence_settings.h"

#include "xensiv_radar_data_management.h"
#include "optimization_list.h"

/*******************************************************************************
* Macros
********************************************************************************/

#define XENSIV_BGT60TRXX_SPI_FREQUENCY      (25000000UL)

/* RTOS tasks */
#define MAIN_TASK_NAME                      "main_task"
#define MAIN_TASK_STACK_SIZE                (configMINIMAL_STACK_SIZE * 4)
#define MAIN_TASK_PRIORITY                  (configMAX_PRIORITIES - 2)
#define PROCESSING_TASK_NAME                "processing_task"
#define PROCESSING_TASK_STACK_SIZE          (configMINIMAL_STACK_SIZE * 4)
#define PROCESSING_TASK_PRIORITY            (configMAX_PRIORITIES - 3)
#define CLI_TASK_NAME                       "cli_task"
#define CLI_TASK_STACK_SIZE                 (configMINIMAL_STACK_SIZE * 20)
#define CLI_TASK_PRIORITY                   (tskIDLE_PRIORITY)

/* Interrupt priorities */
#define GPIO_INTERRUPT_PRIORITY             (7)


/*******************************************************************************
* Function Prototypes
********************************************************************************/
static void main_task(void *pvParameters);
static void processing_task(void *pvParameters);
static void timer_callbak(TimerHandle_t xTimer);

static int32_t init_leds(void);
static int32_t init_sensor(void);
static void process_verbose_cmd(xensiv_radar_presence_handle_t handle, XENSIV_RADAR_PRESENCE_TIMESTAMP time_ms);
static void xensiv_bgt60trxx_interrupt_handler(void* args, cyhal_gpio_event_t event);
void presence_detection_cb(xensiv_radar_presence_handle_t handle,
                           const xensiv_radar_presence_event_t* event,
                           void *data);

/*******************************************************************************
 * Local Declarations
 ********************************************************************************/

typedef struct {
    xensiv_radar_presence_event_t last_reported_event;
    bool verbose;
    XENSIV_RADAR_PRESENCE_TIMESTAMP bookmark_timestamp;
}ce_state_s;


/*******************************************************************************
* Global Variables
********************************************************************************/
static cyhal_spi_t spi_obj;
static xensiv_bgt60trxx_mtb_t bgt60_obj;
static float32_t frame[NUM_SAMPLES_PER_FRAME * 2];
static float32_t avg_chirp[NUM_SAMPLES_PER_CHIRP];

static TaskHandle_t main_task_handler;
static TaskHandle_t processing_task_handler;
static TimerHandle_t timer_handler;
radar_data_manager_s mgr;

ce_state_s ce_app_state;

volatile bool print_job_locked;

/*******************************************************************************
* Function Name: read_radar_data
********************************************************************************
* Summary:
* This is the function for reading radar data using buffering
*
* Parameters:
*  * data: pointer to radar data
*  *num_samples: pointer to number of samples per frame
*  samples_ub: maximum number of samples to be copied at a time from owner task/caller
*
* Return:
*  int32_t: 0 if success
*
*******************************************************************************/
int32_t read_radar_data(uint16_t* data, uint32_t *num_samples, uint32_t samples_ub)
{
    if (xensiv_bgt60trxx_get_fifo_data(&bgt60_obj.dev,
            data,
            NUM_SAMPLES_PER_FRAME) == XENSIV_BGT60TRXX_STATUS_OK)
    {
        *num_samples = NUM_SAMPLES_PER_FRAME *2; // in bytes

        if (samples_ub < NUM_SAMPLES_PER_FRAME *2)
        {
            xensiv_bgt60trxx_soft_reset(&bgt60_obj.dev,XENSIV_BGT60TRXX_RESET_FIFO );
        }
    }

    return 0;
}

/*******************************************************************************
* Function Name: reconf_radar
********************************************************************************
* Summary:
* This is the function for radar reconfiguration
*
* Parameters:
*  requested: Choosed configuration
*
* Return:
*  void
*
*******************************************************************************/
void reconf_radar(optimization_type_e requested)
{
    if (requested == CONFIG_UNINITIALIZED)
    {
        return;
    }

    if (xensiv_bgt60trxx_config(&bgt60_obj.dev,
            optimizations_list[requested].reg_list,
            optimizations_list[requested].reg_list_size) != CY_RSLT_SUCCESS)
    {
        printf("[MSG] ERROR: xensiv_bgt60trxx reconfiguration failed\n");
        CY_ASSERT(0);
    }

    if (xensiv_bgt60trxx_set_fifo_limit(&bgt60_obj.dev,
            optimizations_list[requested].fifo_limit) != CY_RSLT_SUCCESS)
    {
        printf("[MSG] ERROR: xensiv_bgt60trxx set fifo limit failed\n");
        CY_ASSERT(0);
    }

    if (xensiv_bgt60trxx_start_frame(&bgt60_obj.dev, true) != CY_RSLT_SUCCESS)
    {
        printf("[MSG] ERROR: xensiv_bgt60trxx_start_frame failed\n");
        CY_ASSERT(0);
    }
}


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

#ifdef TARGET_APP_CYSBSYSKIT_DEV_01

    /* Initialize the User LED */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

#endif

    mgr.in_read_radar_data = read_radar_data;
    radar_data_manager_init(&mgr, NUM_SAMPLES_PER_FRAME *6, NUM_SAMPLES_PER_FRAME *2);
    radar_data_manager_set_malloc_free(pvPortMalloc,
            vPortFree);

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
    uint32_t sz;
    uint16_t *data_buff = NULL;
    cy_rslt_t result;
    XENSIV_RADAR_PRESENCE_TIMESTAMP last_timestamp = 0;

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

    mgr.subscribe(main_task_handler);

    /* Initialize the initial state of ce_app_state */
    ce_app_state.last_reported_event.state = XENSIV_RADAR_PRESENCE_STATE_ABSENCE;
    ce_app_state.last_reported_event.range_bin = 0;
    ce_app_state.last_reported_event.timestamp = 0;

    if (xensiv_bgt60trxx_start_frame(&bgt60_obj.dev, true) != XENSIV_BGT60TRXX_STATUS_OK)
    {
        CY_ASSERT(0);
    }

    for(;;)
    {
        /* Wait for the GPIO interrupt to indicate that another slice is available */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        mgr.read_from_buffer(1, &data_buff, &sz);

        /* Data preprocessing */
        uint16_t *bgt60_buffer_ptr = data_buff;
        float32_t *frame_ptr = &frame[0];
        for (int32_t sample = 0; sample < NUM_SAMPLES_PER_FRAME * 2; ++sample)
        {
            *frame_ptr++ = ((float32_t)(*bgt60_buffer_ptr++) / 4096.0F);
        }

        mgr.ack_data_read(1);

        /* calculate the average of the chirps first */
        arm_fill_f32(0, avg_chirp, NUM_SAMPLES_PER_CHIRP);

        for (int chirp = 0; chirp < NUM_CHIRPS_PER_FRAME * 2; chirp++)
        {
            arm_add_f32(avg_chirp, &frame[NUM_SAMPLES_PER_CHIRP * chirp], avg_chirp, NUM_SAMPLES_PER_CHIRP);
        }

        arm_scale_f32(avg_chirp, 1.0f / NUM_CHIRPS_PER_FRAME, avg_chirp, NUM_SAMPLES_PER_CHIRP);

        if(ce_app_state.last_reported_event.timestamp != last_timestamp)
        {
            last_timestamp = ce_app_state.last_reported_event.timestamp; // save latest timestamp
            result = radar_config_optimize(ce_app_state.last_reported_event.state);
            if(result != ESTATUS_SUCCESS)
            {
                printf("[MSG] ERROR: radar_config_optimize failed with error %" PRIi32 "\n", result);
                CY_ASSERT(0);
            }
        }

        /* Tell processing task to take over */
        xTaskNotifyGive(processing_task_handler);

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

    xensiv_radar_presence_handle_t handle;
    cy_rslt_t result;

    xensiv_radar_presence_set_malloc_free(pvPortMalloc,
                                          vPortFree);

    if (xensiv_radar_presence_alloc(&handle, &default_config) != 0)
    {
        CY_ASSERT(0);
    }

    xensiv_radar_presence_set_callback(handle, presence_detection_cb, NULL);
    result = radar_config_optimizer_init(reconf_radar);

    if(result != ESTATUS_SUCCESS)
    {
        printf("[MSG] ERROR: radar_config_optimizer_init failed with error %" PRIi32 "\n", result);
        CY_ASSERT(0);
    }

    result = radar_config_optimizer_set_operational_mode(default_config.mode);

    if(result != ESTATUS_SUCCESS)
    {
        printf("[MSG] ERROR: radar_config_optimizer_set_operational_mode failed with error %" PRIi32 "\n", result);
        CY_ASSERT(0);
    }

    if (xTaskCreate(console_task, CLI_TASK_NAME, CLI_TASK_STACK_SIZE, handle, CLI_TASK_PRIORITY, NULL) != pdPASS)
    {
        CY_ASSERT(0);
    }

    for(;;)
    {
        /* Wait for frame data available to process */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        xensiv_radar_presence_process_frame(handle, avg_chirp, xTaskGetTickCount() * portTICK_PERIOD_MS);
        process_verbose_cmd(handle, xTaskGetTickCount() * portTICK_PERIOD_MS);
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

    if (!ce_app_state.verbose)
    {
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
                printf("[MSG] ERROR: Unknown reported state in event handling\n");
                break;
        }

    }

    /* save the last reported event state */
    ce_app_state.last_reported_event = *event;
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
        printf("[MSG] ERROR: cyhal_spi_init failed\n");
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
        printf("[MSG] ERROR: cyhal_spi_set_frequency failed\n");
        return -1;
    }

    /* Enable LDO */
    if (cyhal_gpio_init(PIN_XENSIV_BGT60TRXX_LDO_EN,
                        CYHAL_GPIO_DIR_OUTPUT,
                        CYHAL_GPIO_DRIVE_STRONG,
                        true) != CY_RSLT_SUCCESS)
    {
        printf("[MSG] ERROR: LDO_EN cyhal_gpio_init failed\n");
        return -1;
    }

    /* Wait LDO stable */
    (void)cyhal_system_delay_ms(5);

    if (xensiv_bgt60trxx_mtb_init(&bgt60_obj, 
                                  &spi_obj, 
                                  PIN_XENSIV_BGT60TRXX_SPI_CSN, 
                                  PIN_XENSIV_BGT60TRXX_RSTN, 
                                  register_list_micro_only, // default config with macro settings
                                  XENSIV_BGT60TRXX_CONF_NUM_REGS_MACRO) != CY_RSLT_SUCCESS)
    {
        printf("[MSG] ERROR: xensiv_bgt60trxx_mtb_init failed\n");
        return -1;
    }

    if (xensiv_bgt60trxx_mtb_interrupt_init(&bgt60_obj,
                                            NUM_SAMPLES_PER_FRAME*2,
                                            PIN_XENSIV_BGT60TRXX_IRQ,
                                            GPIO_INTERRUPT_PRIORITY,
                                            xensiv_bgt60trxx_interrupt_handler,
                                            NULL) != CY_RSLT_SUCCESS)
    {
        printf("[MSG] ERROR: xensiv_bgt60trxx_mtb_interrupt_init failed\n");
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

    mgr.run(true);
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
        printf("[MSG] ERROR: GPIO initialization for LED_RGB_RED failed\n");
        return -1;
    }

    if( cyhal_gpio_init(LED_RGB_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false)!= CY_RSLT_SUCCESS)
    {
        printf("[MSG] ERROR: GPIO initialization for LED_RGB_GREEN failed\n");
        return -1;
    }

    if( cyhal_gpio_init(LED_RGB_BLUE, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false)!= CY_RSLT_SUCCESS)
    {
        printf("[MSG] ERROR: GPIO initialization for LED_RGB_BLUE failed\n");
        return -1;
    }

    return 0;
}


/*******************************************************************************
 * Function Name: process_verbose_cmd
 ********************************************************************************
 * Summary:
 * This function processes the prints the desired output on CLI provided
 * user has set the "verbose enabled" through Cli task.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  none
 *
 *******************************************************************************/
void process_verbose_cmd(xensiv_radar_presence_handle_t handle,
        XENSIV_RADAR_PRESENCE_TIMESTAMP time_ms)
{
    float32_t energy = 0;
    int range_bin = 0;

    if (ce_app_state.verbose == false)
    {
        return;
    }

    if (ce_app_state.bookmark_timestamp + 1000 <= time_ms)
    {
        print_job_locked = true;

        switch (ce_app_state.last_reported_event.state)
        {
            case XENSIV_RADAR_PRESENCE_STATE_MACRO_PRESENCE:
                cyhal_gpio_write(LED_RGB_RED, true);
                cyhal_gpio_write(LED_RGB_GREEN, false);
                printf("[INFO] macro presence %" PRIi32 " %" PRIi32 "\n",
                        ce_app_state.last_reported_event.range_bin,
                        time_ms);
                break;

            case XENSIV_RADAR_PRESENCE_STATE_MICRO_PRESENCE:
                cyhal_gpio_write(LED_RGB_RED, true);
                cyhal_gpio_write(LED_RGB_GREEN, false);
                printf("[INFO] micro presence %" PRIi32 " %" PRIi32 "\n",
                        ce_app_state.last_reported_event.range_bin,
                        time_ms);
                break;

            case XENSIV_RADAR_PRESENCE_STATE_ABSENCE:
                cyhal_gpio_write(LED_RGB_RED, false);
                cyhal_gpio_write(LED_RGB_GREEN, true);
                printf("[INFO] absence %" PRIu32 "\n", time_ms);
                break;

            default:
                printf("[MSG] ERROR: Unknown reported state in event handling\n");
                break;
        }

        const cfloat32_t *macro_fft_buff = xensiv_radar_presence_get_macro_fft_buffer(handle);

        printf("[MACRO_FFT] %lu",(unsigned long)time_ms);

        for(int i = 0; i< MACRO_FFT_BUFF_SIZE; i++)
        {
            float zero[2] = { 0.f, 0.f};

            printf("%lf ", arm_euclidean_distance_f32((float*)&macro_fft_buff[i], zero, 2));
        }

        printf("\n");

        xensiv_radar_presence_get_max_macro(handle, &energy, &range_bin);
        printf("[MACRO] %d %lf %lu\n", range_bin, energy, (unsigned long)time_ms);

        xensiv_radar_presence_get_max_micro(handle, &energy, &range_bin);
        printf("[MICRO] %d %lf %lu\n", range_bin, energy, (unsigned long) time_ms);

        ce_app_state.bookmark_timestamp = time_ms;

        print_job_locked = false;
    }
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

#ifdef TARGET_APP_CYSBSYSKIT_DEV_01
    cyhal_gpio_toggle(CYBSP_USER_LED);
#endif
}

/* [] END OF FILE */
