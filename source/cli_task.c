/*****************************************************************************
 * File name: cli_task.c
 *
 * Description: This file implements a terminal UI to configure parameters
 ** for radar presence application.

 *
 * ===========================================================================
 * Copyright (C) 2023 Infineon Technologies AG. All rights reserved.
 * ===========================================================================
 *
 * ===========================================================================
 * Infineon Technologies AG (INFINEON) is supplying this file for use
 * exclusively with Infineon's sensor products. This file can be freely
 * distributed within development tools and software supporting such
 * products.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * INFINEON SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON
 * WHATSOEVER.
 * ===========================================================================
 */

#include <stdlib.h>

#include "cy_retarget_io.h"

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_CLI.h"

#include "cli_task.h"

#include "xensiv_radar_presence.h"
#include "radar_config_optimizer.h"

/*******************************************************************************
 * Macros
 ********************************************************************************/
#define NUMBER_OF_COMMANDS (9)

/* Strings length */
#define MAX_INPUT_LENGTH  (50)
#define MAX_OUTPUT_LENGTH (100)

/* Strings for enable and disable */
#define ENABLE_STRING  ("enable")
#define DISABLE_STRING ("disable")

/* Max range min - max */
#define MAX_RANGE_MIN_LIMIT (0.66f)
#define MAX_RANGE_MAX_LIMIT (5.0f)

/* Macro threshold min - max */
#define MACRO_THRESHOLD_MIN_LIMIT (0.5f)
#define MACRO_THRESHOLD_MAX_LIMIT (2.0f)

/* Micro threshold min - max */
#define MICRO_THRESHOLD_MIN_LIMIT (0.2f)
#define MICRO_THRESHOLD_MAX_LIMIT (50.0f)

/* Names for presence mode */
#define MACRO_ONLY_STRING      ("macro_only")
#define MICRO_ONLY_STRING      ("micro_only")
#define MICRO_IF_MACRO_STRING  ("micro_if_macro")
#define MICRO_AND_MACRO_STRING ("micro_and_macro")

/* Keyboard keys */
#define ENTER_KEY     (0x0D)
#define ESC_KEY       (0x1B)
#define BACKSPACE_KEY (0x08)

/*******************************************************************************
 * Local Declarations
 ********************************************************************************/
typedef struct
{
    xensiv_radar_presence_event_t last_reported_event;
    bool verbose;
    XENSIV_RADAR_PRESENCE_TIMESTAMP bookmark_timestamp;
}ce_state_s;

/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
static BaseType_t set_max_range(char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString);
static BaseType_t set_macro_threshold(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t set_micro_threshold(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t turn_bandpass_filter(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t turn_decimation_filter(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t set_presence_mode(char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString);
static BaseType_t display_board_Info(char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString);
static BaseType_t display_solution_config(char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString); 
static BaseType_t set_verbose(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString);
static inline bool check_bool_validation(const char *value, const char *enable,
        const char *disable);
static inline bool check_float_validation(float32_t value, float32_t min,
        float32_t max);
static inline bool string_to_bool(const char *string, const char *enable,
        const char *disable);
static inline bool check_mode_validation(const char *mode);
static inline xensiv_radar_presence_mode_t string_to_mode(const char *mode);
extern void presence_detection_cb(xensiv_radar_presence_handle_t handle,
        const xensiv_radar_presence_event_t *event, void *data);

/*******************************************************************************
 * Variables
 ********************************************************************************/
static const CLI_Command_Definition_t command_list[NUMBER_OF_COMMANDS] =
{
    {
        .pcCommand = "set_max_range",
        .pcHelpString = "set_max_range <value> - Sets the max range for presence algorithm in meters. Range <0.66-5.0>\n",
        .pxCommandInterpreter = set_max_range,
        .cExpectedNumberOfParameters = 1
    },
    {
        .pcCommand = "set_macro_threshold",
        .pcHelpString = "set_macro_threshold <value> - Sets macro threshold for presence algorithm. Range <0.5-2.0>\n",
        .pxCommandInterpreter = set_macro_threshold,
        .cExpectedNumberOfParameters = 1
    },
    {
        .pcCommand = "set_micro_threshold",
        .pcHelpString = "set_micro_threshold <value> - Sets micro threshold for presence algorithm. Range <0.2-50.0>\n",
        .pxCommandInterpreter = set_micro_threshold,
        .cExpectedNumberOfParameters = 1
    },
    {
        .pcCommand = "set_bandpass_filter",
        .pcHelpString = "set_bandpass_filter <enable|disable> - Enabling/disabling bandpass filter\n",
        .pxCommandInterpreter = turn_bandpass_filter,
        .cExpectedNumberOfParameters = 1
    },
    {
        .pcCommand = "set_decimation_filter",
        .pcHelpString = "set_decimation_filter <enable|disable> - Enabling/disabling decimation filter\n",
        .pxCommandInterpreter = turn_decimation_filter,
        .cExpectedNumberOfParameters = 1
    },
    {
        .pcCommand = "set_mode",
        .pcHelpString = "set_mode <macro_only|micro_only|micro_if_macro|micro_and_macro> - Chooses work mode\n",
        .pxCommandInterpreter = set_presence_mode,
        .cExpectedNumberOfParameters = 1
    },
    {
        .pcCommand = "verbose",
        .pcHelpString = "verbose <enable|disable> - Enable/disable detailed verbose status to be updated every second\n",
        .pxCommandInterpreter = set_verbose,
        .cExpectedNumberOfParameters = 1
    },
    {
        .pcCommand = "board_info",
        .pcHelpString = "board_info -  Board_Information\n",
        .pxCommandInterpreter = display_board_Info,
        .cExpectedNumberOfParameters = 0
    },
    {
        .pcCommand = "config",
        .pcHelpString = "config - solution configuration information\n",
        .pxCommandInterpreter = display_solution_config,
        .cExpectedNumberOfParameters = 0
    }
};

static xensiv_radar_presence_handle_t handle;
extern ce_state_s ce_app_state;

/*******************************************************************************
 * Function Name: console_task
 ********************************************************************************
 * Summary:
 * This is the console task.
 *    1. Register commands
 *    2. In loop there are two modes: presence when the events are detected and setting_mode, when the user can change
 *work parameters
 *       - Waits for a sign
 *       - When ENTER is hit go to the settings mode
 *       - Use 'help' command to know what commands are available
 *       - Type commands with values to change parameters
 *       - Press ESC to exit settings mode and go again to presence mode
 *
 * Parameters:
 *  void
 *
 * Return:
 *  None
 *
 *******************************************************************************/
__NO_RETURN void console_task(void *pvParameters)
{
    int8_t cInputIndex = 0;
    BaseType_t xMoreDataToFollow;
    /* The input and output buffers are declared static to keep them off the stack. */
    static char pcOutputString[MAX_OUTPUT_LENGTH];
    static char pcInputString[MAX_INPUT_LENGTH];
    bool setting_mode = false;

    handle = (xensiv_radar_presence_handle_t) pvParameters;
    CY_ASSERT(handle != NULL);

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    for (int32_t i = 0; i < NUMBER_OF_COMMANDS; ++i)
    {
        FreeRTOS_CLIRegisterCommand(&command_list[i]);
    }

    for (;;)
    {
        /* Wait for a sign */
        int32_t c = getchar();

        if (!setting_mode)
        {
            /* Enter setting mode */
            if (c == ENTER_KEY)
            {
                cInputIndex = 0;
                memset(pcInputString, 0x00, MAX_INPUT_LENGTH);
                setting_mode = true;
                xensiv_radar_presence_set_callback(handle, NULL, NULL);
                printf("\r\nEnter setting mode and stop processing\r\n"
                        "> ");
            }
        }
        else
        {
            /* Exit setting mode */
            if (c == ESC_KEY)
            {
                cInputIndex = 0;
                memset(pcInputString, 0x00, MAX_INPUT_LENGTH);
                setting_mode = false;
                printf("\r\nQuit from settings menu and back to processing\r\n\n");
                xensiv_radar_presence_set_callback(handle, presence_detection_cb, NULL);
            }
            else if (c == ENTER_KEY) // confirm entered text
            {
                putchar('\n');

                /* The command interpreter is called repeatedly until it returns
                 pdFALSE.  See the "Implementing a command" documentation for an
                 explanation of why this is. */
                do {
                    /* Send the command string to the command interpreter.  Any
                     output generated by the command interpreter will be placed in the
                     pcOutputString buffer. */
                    xMoreDataToFollow = FreeRTOS_CLIProcessCommand(
                            pcInputString, /* The command string.*/
                            pcOutputString, /* The output buffer. */
                            MAX_OUTPUT_LENGTH /* The size of the output buffer. */
                    );

                    /* Write the output generated by the command interpreter to the
                     console. */
                    printf("%s", pcOutputString);
                } while (xMoreDataToFollow != pdFALSE);

                /* All the strings generated by the input command have been sent.
                 Processing of the command is complete.  Clear the input string ready
                 to receive the next command. */
                cInputIndex = 0;
                memset(pcInputString, 0x00, MAX_INPUT_LENGTH);

                printf("> ");
            }
            else
            {
                if (c == BACKSPACE_KEY)
                {
                    /* Backspace was pressed.  Erase the last character in the input
                     buffer - if there are any. */
                    if (cInputIndex > 0)
                    {
                        cInputIndex--;
                        pcInputString[cInputIndex] = ' ';
                        putchar(BACKSPACE_KEY);
                    }
                }
                else
                {
                    /* A character was entered.  It was not a new line, backspace
                     or carriage return, so it is accepted as part of the input and
                     placed into the input buffer.  When a \n is entered the complete
                     string will be passed to the command interpreter. */
                    if (cInputIndex < MAX_INPUT_LENGTH)
                    {
                        pcInputString[cInputIndex] = c;
                        cInputIndex++;
                        putchar(c);
                    }
                }
            }
        }
    }
}

/*******************************************************************************
 * Function Name: set_max_range
 ********************************************************************************
 * Summary:
 *   Sets the max range for presence detection algorithm
 *
 * Parameters:
 *   pcWriteBuffer: buffer into which the output from executing the command can be written
 *   xWriteBufferLen:length, in bytes of the pcWriteBuffer buffer
 *   pcCommandString: entire string as input by
 the user (from which parameters can be extracted)
 *
 * Return:
 *   pdFALSE indicating that the function ends it's processing
 *******************************************************************************/
static BaseType_t set_max_range(char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    float32_t float_value = 0.0;
    xensiv_radar_presence_config_t config;
    const char *pcParameter;
    BaseType_t lParameterStringLength;
    float32_t maxRange;

    configASSERT(pcWriteBuffer);

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
            1, /* Return the first parameter. */
            &lParameterStringLength); /* Store the parameter string length. */

    configASSERT(pcParameter);
    result = xensiv_radar_presence_get_config(handle, &config);

    if (result != XENSIV_RADAR_PRESENCE_OK)
    {
        sprintf(pcWriteBuffer, "Error while reading presence config\r\n");
    }
    else
    {
        float_value = strtof(pcParameter, NULL);

        if (check_float_validation(float_value, MAX_RANGE_MIN_LIMIT, MAX_RANGE_MAX_LIMIT))
        {
            config.max_range_bin = (int32_t) (float_value
                    / (xensiv_radar_presence_get_bin_length(handle)));
            vTaskSuspendAll();
            result = xensiv_radar_presence_set_config(handle, &config);
            xensiv_radar_presence_reset(handle);
            xTaskResumeAll();
            maxRange = (xensiv_radar_presence_get_bin_length(handle)* config.max_range_bin);

            if (result != XENSIV_RADAR_PRESENCE_OK)
            {
                sprintf(pcWriteBuffer, "Error while setting new config.\r\n\n");
            }
            else
            {
                sprintf(pcWriteBuffer," [CONFIG] max_range %f \r\n\n", maxRange);
            }
        }
        else
        {
            sprintf(pcWriteBuffer, "Invalid value.\r\n\n");
        }
    }

    return pdFALSE;
}

/*******************************************************************************
 * Function Name: set_macro_threshold
 ********************************************************************************
 * Summary:
 *   Sets macro threshold for presence detection algorithm
 *
 * Parameters:
 *   pcWriteBuffer: buffer into which the output from executing the command can be written
 *   xWriteBufferLen:length, in bytes of the pcWriteBuffer buffer
 *   pcCommandString: entire string as input by
 the user (from which parameters can be extracted)
 *
 * Return:
 *   pdFALSE indicating that the function ends it's processing
 *******************************************************************************/
static BaseType_t set_macro_threshold(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    float32_t float_value = 0.0;
    xensiv_radar_presence_config_t config;
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    configASSERT(pcWriteBuffer);

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
            1, /* Return the first parameter. */
            &lParameterStringLength); /* Store the parameter string length. */

    configASSERT(pcParameter);
    result = xensiv_radar_presence_get_config(handle, &config);

    if (result != XENSIV_RADAR_PRESENCE_OK)
    {
        sprintf(pcWriteBuffer, "Error while reading presence config\r\n");
    }
    else
    {
        float_value = strtof(pcParameter, NULL);

        if (check_float_validation(float_value, MACRO_THRESHOLD_MIN_LIMIT,
                MACRO_THRESHOLD_MAX_LIMIT))
        {
            config.macro_threshold = float_value;
            vTaskSuspendAll();
            result = xensiv_radar_presence_set_config(handle, &config);
            xensiv_radar_presence_reset(handle);
            xTaskResumeAll();

            if (result != XENSIV_RADAR_PRESENCE_OK)
            {
                sprintf(pcWriteBuffer, "Error while setting new config.\r\n\n");
            }
            else
            {
                sprintf(pcWriteBuffer, "[CONFIG] macro_threshold %f \r\n\n",config.macro_threshold);
            }
        }
        else
        {
            sprintf(pcWriteBuffer, "Invalid value.\r\n\n");
        }
    }

    return pdFALSE;
}

/*******************************************************************************
 * Function Name: set_micro_threshold
 ********************************************************************************
 * Summary:
 *   Sets micro threshold for presence detection algorithm
 *
 * Parameters:
 *   pcWriteBuffer: buffer into which the output from executing the command can be written
 *   xWriteBufferLen:length, in bytes of the pcWriteBuffer buffer
 *   pcCommandString: entire string as input by
 the user (from which parameters can be extracted)
 *
 * Return:
 *   pdFALSE indicating that the function ends it's processing
 *******************************************************************************/
static BaseType_t set_micro_threshold(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    float32_t float_value = 0.0;
    xensiv_radar_presence_config_t config;
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    configASSERT(pcWriteBuffer);

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
            1, /* Return the first parameter. */
            &lParameterStringLength); /* Store the parameter string length. */

    configASSERT(pcParameter);
    result = xensiv_radar_presence_get_config(handle, &config);

    if (result != XENSIV_RADAR_PRESENCE_OK)
    {
        sprintf(pcWriteBuffer, "Error while reading presence config\r\n");
    }
    else
    {
        float_value = strtof(pcParameter, NULL);

        if (check_float_validation(float_value, MICRO_THRESHOLD_MIN_LIMIT,
                MICRO_THRESHOLD_MAX_LIMIT))
        {
            config.micro_threshold = float_value;
            vTaskSuspendAll();
            result = xensiv_radar_presence_set_config(handle, &config);
            xensiv_radar_presence_reset(handle);
            xTaskResumeAll();

            if (result != XENSIV_RADAR_PRESENCE_OK)
            {
                sprintf(pcWriteBuffer, "Error while setting new config.\r\n\n");
            }
            else
            {
                sprintf(pcWriteBuffer, "[CONFIG] micro_threshold %f \r\n\n",config.micro_threshold);
            }
        }
        else
        {
            sprintf(pcWriteBuffer, "Invalid value.\r\n\n");
        }
    }

    return pdFALSE;
}

/*******************************************************************************
 * Function Name: turn_bandpass_filter
 ********************************************************************************
 * Summary:
 *   Turning on/off bandpass filter for presence detection algorithm
 *
 * Parameters:
 *   pcWriteBuffer: buffer into which the output from executing the command can be written
 *   xWriteBufferLen:length, in bytes of the pcWriteBuffer buffer
 *   pcCommandString: entire string as input by
 the user (from which parameters can be extracted)
 *
 * Return:
 *   pdFALSE indicating that the function ends it's processing
 *******************************************************************************/
static BaseType_t turn_bandpass_filter(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    xensiv_radar_presence_config_t config;
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    configASSERT(pcWriteBuffer);

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
            1, /* Return the first parameter. */
            &lParameterStringLength); /* Store the parameter string length. */

    configASSERT(pcParameter);
    result = xensiv_radar_presence_get_config(handle, &config);

    if (result != XENSIV_RADAR_PRESENCE_OK)
    {
        sprintf(pcWriteBuffer, "Error while reading presence config\r\n");
    }
    else
    {
        if (check_bool_validation(pcParameter, ENABLE_STRING, DISABLE_STRING))
        {
            config.macro_fft_bandpass_filter_enabled = string_to_bool(
                    pcParameter, ENABLE_STRING, DISABLE_STRING);
            vTaskSuspendAll();
            result = xensiv_radar_presence_set_config(handle, &config);
            xensiv_radar_presence_reset(handle);
            xTaskResumeAll();
            if (result != XENSIV_RADAR_PRESENCE_OK)
            {
                sprintf(pcWriteBuffer, "Error while setting new config.\r\n\n");
            }
            else
            {
                sprintf(pcWriteBuffer, "[CONFIG] bandpass_filter %s \r\n\n", pcParameter);
            }
        }
        else
        {
            sprintf(pcWriteBuffer, "Invalid value.\r\n\n");
        }
    }

    return pdFALSE;
}

/*******************************************************************************
 * Function Name: turn_decimation_filter
 ********************************************************************************
 * Summary:
 *   Turning on/off decimation filter for presence detection algorithm
 *
 * Parameters:
 *   pcWriteBuffer: buffer into which the output from executing the command can be written
 *   xWriteBufferLen:length, in bytes of the pcWriteBuffer buffer
 *   pcCommandString: entire string as input by
 the user (from which parameters can be extracted)
 *
 * Return:
 *   pdFALSE indicating that the function ends it's processing
 *******************************************************************************/
static BaseType_t turn_decimation_filter(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    xensiv_radar_presence_config_t config;
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    configASSERT(pcWriteBuffer);

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
            1, /* Return the first parameter. */
            &lParameterStringLength); /* Store the parameter string length. */

    configASSERT(pcParameter);
    result = xensiv_radar_presence_get_config(handle, &config);

    if (result != XENSIV_RADAR_PRESENCE_OK)
    {
        sprintf(pcWriteBuffer, "Error while reading presence config\r\n");
    }
    else
    {
        if (check_bool_validation(pcParameter, ENABLE_STRING, DISABLE_STRING))
        {
            config.micro_fft_decimation_enabled = string_to_bool(pcParameter,
                    ENABLE_STRING, DISABLE_STRING);
            vTaskSuspendAll();
            result = xensiv_radar_presence_set_config(handle, &config);
            xensiv_radar_presence_reset(handle);
            xTaskResumeAll();

            if (result != XENSIV_RADAR_PRESENCE_OK)
            {
                sprintf(pcWriteBuffer, "Error while setting new config.\r\n\n");
            }
            else
            {
                sprintf(pcWriteBuffer, "[CONFIG] decimation_filter %s \r\n\n", pcParameter);
            }
        }
        else
        {
            sprintf(pcWriteBuffer, "Invalid value.\r\n\n");
        }
    }

    return pdFALSE;
}

/*******************************************************************************
 * Function Name: set_presence_mode
 ********************************************************************************
 * Summary:
 *   Setting mode for presence detection algorithm
 *
 * Parameters:
 *   pcWriteBuffer: buffer into which the output from executing the command can be written
 *   xWriteBufferLen:length, in bytes of the pcWriteBuffer buffer
 *   pcCommandString: entire string as input by
 the user (from which parameters can be extracted)
 *
 * Return:
 *   pdFALSE indicating that the function ends it's processing
 *******************************************************************************/
static BaseType_t set_presence_mode(char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    xensiv_radar_presence_config_t config;
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    configASSERT(pcWriteBuffer);

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
            1, /* Return the first parameter. */
            &lParameterStringLength); /* Store the parameter string length. */

    configASSERT(pcParameter);
    result = xensiv_radar_presence_get_config(handle, &config);

    if (result != XENSIV_RADAR_PRESENCE_OK)
    {
        sprintf(pcWriteBuffer, "Error while reading presence config\r\n");
    }
    else
    {
        if (check_mode_validation(pcParameter))
        {
            xensiv_radar_presence_mode_t mode = string_to_mode(pcParameter);

            config.mode = mode;
            vTaskSuspendAll();
            result = xensiv_radar_presence_set_config(handle, &config);
            xensiv_radar_presence_reset(handle);
            xTaskResumeAll();

            result = radar_config_optimizer_set_operational_mode(mode);
            if(result != ESTATUS_SUCCESS)
            {
                sprintf(pcWriteBuffer, "Error while setting new operational mode.\r\n\n");
            }

            if (result != XENSIV_RADAR_PRESENCE_OK)
            {
                sprintf(pcWriteBuffer, "Error while setting new config.\r\n\n");
            }
            else
            {
                sprintf(pcWriteBuffer, "[CONFIG] set_mode %s \r\n\n", pcParameter);
            }
        }
        else
        {
            sprintf(pcWriteBuffer, "Invalid value.\r\n\n");
        }
    }

    return pdFALSE;
}


/*******************************************************************************
 * Function Name: set_verbose
 ********************************************************************************
 * Summary:
 *   Turning on/off bandpass filter for presence detection algorithm
 *
 * Parameters:
 *   pcWriteBuffer: buffer into which the output from executing the command can be written
 *   xWriteBufferLen:length, in bytes of the pcWriteBuffer buffer
 *   pcCommandString: entire string as input by
 the user (from which parameters can be extracted)
 *
 * Return:
 *   pdFALSE indicating that the function ends it's processing
 *******************************************************************************/
static BaseType_t set_verbose(char *pcWriteBuffer,
        size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    configASSERT(pcWriteBuffer);

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
            1, /* Return the first parameter. */
            &lParameterStringLength); /* Store the parameter string length. */

    configASSERT(pcParameter);
    if (check_bool_validation(pcParameter, ENABLE_STRING, DISABLE_STRING))
    {
        ce_app_state.verbose = string_to_bool(pcParameter,
                ENABLE_STRING, DISABLE_STRING);
        sprintf(pcWriteBuffer, "ok\n");
    }
    else
    {
        sprintf(pcWriteBuffer, "Invalid value.\r\n\n");
    }

    return pdFALSE;

}

/*******************************************************************************
 * Function Name: display_device_Info
 ********************************************************************************
 * Summary:
 *   Query for device information
 *
 * Parameters:
 *   pcWriteBuffer: buffer into which the output from executing the command can be written
 *   xWriteBufferLen:length, in bytes of the pcWriteBuffer buffer
 *   pcCommandString: entire string as input by the user (from which parameters can be extracted)
 *
 * Return:
 *   pdFALSE indicating that the function ends it's processing
 *******************************************************************************/
static BaseType_t display_board_Info(char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString)
{
    printf(BOARD_INFO);
    printf("\n");
    printf(BOARD_INFO_APPLICATION);
    printf("\n");
    printf(BOARD_INFO_FIRMWARE);
    printf("\n");
    printf(BOARD_INFO_DEVICE_NAME);
    printf("\n");
    printf(BOARD_INFO_DEVICE_VERSION);
    printf("\n");
    printf(BOARD_INFO);
    sprintf(pcWriteBuffer, "\n");

    return pdFALSE;
}

/*******************************************************************************

 * Function Name: display_solution_config
 ********************************************************************************
 * Summary:
 *   display solution configuration
 *
 * Parameters:
 *   pcWriteBuffer: buffer into which the output from executing the command can be written
 *   xWriteBufferLen:length, in bytes of the pcWriteBuffer buffer
 *   pcCommandString: entire string as input by
 the user (from which parameters can be extracted)
 *
 * Return:
 *   pdFALSE indicating that the function ends it's processing
 *******************************************************************************/

static BaseType_t display_solution_config(char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    xensiv_radar_presence_config_t config;
    float32_t maxRange;
    float32_t minRange;

    result = xensiv_radar_presence_get_config(handle, &config);

    maxRange = (xensiv_radar_presence_get_bin_length(handle)* config.max_range_bin);
    minRange = (xensiv_radar_presence_get_bin_length(handle)* config.min_range_bin);

    const char* mode = MACRO_ONLY_STRING;
    if(config.mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_ONLY) 
    {
        mode = MICRO_ONLY_STRING;
    } 
    else if(config.mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO)
    {
        mode = MICRO_IF_MACRO_STRING;
    }
    else if(config.mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_AND_MACRO)
    {
        mode = MICRO_AND_MACRO_STRING;
    }

    if (result != XENSIV_RADAR_PRESENCE_OK)
    {
        sprintf(pcWriteBuffer, "Error while reading presence config\r\n");
    }
    else 
    {
        printf(CONFIG);
        printf("\n");
        printf(CONFIG_MODE);
        printf("%s",mode);
        printf("\n");   
        printf(CONFIG_MAX_RANGE);
        printf("%f",maxRange);
        printf("\n");   
        printf(CONFIG_MIN_RANGE);
        printf("%f",minRange);
        printf("\n");
        printf(CONFIG_MACRO_THRESHOLD);
        printf("%f",config.macro_threshold);
        printf("\n");
        printf(CONFIG_MICRO_THRESHOLD);
        printf("%f",config.micro_threshold);
        printf("\n");
        printf(CONFIG_BANDPASS_FILTER);
        (config.macro_fft_bandpass_filter_enabled == true)?printf("enable"):printf("disable");
        printf("\n");
        printf(CONFIG_DECIMATION_FILTER);
        (config.micro_fft_decimation_enabled == true)?printf("enable"):printf("disable");
        printf("\n");

        printf(CONFIG);
        sprintf(pcWriteBuffer, "\n");
    }

    return pdFALSE;
}


/*******************************************************************************
 * Function Name: check_bool_validation
 ********************************************************************************
 * Summary:
 *   Checks if entered value is a proper string for enable or disable
 *
 * Parameters:
 *   value : Entered string value
 *   enable : String value for enable
 *   disable : String value for disable
 *
 * Return:
 *   True if the value is same as enable or disable value, false if it is different
 *******************************************************************************/
static inline bool check_bool_validation(const char *value, const char *enable,
        const char *disable)
{
    bool result = false;

    if ((strcmp(value, enable) == 0) || (strcmp(value, disable) == 0))
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/*******************************************************************************
 * Function Name: check_float_validation
 ********************************************************************************
 * Summary:
 *   Checks if entered value is within a range
 *
 * Parameters:
 *   value : Entered float value
 *   min : Minimum value
 *   max : Maximum value
 *
 * Return:
 *   True if the value is within a range, false if not
 *******************************************************************************/
static inline bool check_float_validation(float32_t value, float32_t min, float32_t max)
{
    bool result = false;

    if ((value >= min) && (value <= max))
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/*******************************************************************************
 * Function Name: string_to_bool
 ********************************************************************************
 * Summary:
 *   Converts string value to true or false
 *
 * Parameters:
 *   string : Entered string value
 *   enable : String for enable value
 *   disable : String for disable value
 *
 * Return:
 *   True if entered string is enable, false if disable
 *******************************************************************************/
static inline bool string_to_bool(const char *string, const char *enable,
        const char *disable)
{
    assert((strcmp(string, enable) == 0) || (strcmp(string, disable) == 0));
    bool result = false;

    if (strcmp(string, enable) == 0)
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}


/*******************************************************************************
 * Function Name: check_mode_validation
 ********************************************************************************
 * Summary:
 *   Checks if entered value is a proper mode value
 *
 * Parameters:
 *   value : Entered mode value
 *
 * Return:
 *   True if the value is correct, false if the value is incorrect
 *******************************************************************************/
static inline bool check_mode_validation(const char *mode)
{
    bool result = false;

    if ((strcmp(mode, MACRO_ONLY_STRING) == 0)
            || (strcmp(mode, MICRO_ONLY_STRING) == 0)
            || (strcmp(mode, MICRO_IF_MACRO_STRING) == 0)
            || (strcmp(mode, MICRO_AND_MACRO_STRING) == 0))
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/*******************************************************************************
 * Function Name: string_to_mode
 ********************************************************************************
 * Summary:
 *   Translates string value into a numeral value for mode
 *
 * Parameters:
 *   value : Entered string
 *
 * Return:
 *   Presence mode value
 *******************************************************************************/
static inline xensiv_radar_presence_mode_t string_to_mode(const char *mode)
{
    xensiv_radar_presence_mode_t result = XENSIV_RADAR_PRESENCE_MODE_MACRO_ONLY;

    if (strcmp(mode, MACRO_ONLY_STRING) == 0)
    {
        result = XENSIV_RADAR_PRESENCE_MODE_MACRO_ONLY;
    }
    else if (strcmp(mode, MICRO_ONLY_STRING) == 0)
    {
        result = XENSIV_RADAR_PRESENCE_MODE_MICRO_ONLY;
    }
    else if (strcmp(mode, MICRO_IF_MACRO_STRING) == 0)
    {
        result = XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO;
    }
    else if (strcmp(mode, MICRO_AND_MACRO_STRING) == 0)
    {
        result = XENSIV_RADAR_PRESENCE_MODE_MICRO_AND_MACRO;
    }
    else
    {

    }

    return result;
}
