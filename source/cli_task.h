/******************************************************************************
** File name: cli_task.h
**
** Description: This file contains the function prototypes and constants used
**   in cli_task.c.
**
*******************************************************************************
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company) or
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

#ifndef CLI_TASK_H_
#define CLI_TASK_H_


/*******************************************************************************
 * Functions
 *******************************************************************************/

__NO_RETURN void console_task(void *pvParameters);

#define BOARD_INFO                     ("[BOARD_INFO]")
#define BOARD_INFO_APPLICATION         ("[BOARD_INFO] application PSoC 6 MCU : Human presence detection")
#define BOARD_INFO_FIRMWARE            ("[BOARD_INFO] firmware 1.1.0")
#define BOARD_INFO_DEVICE_NAME         ("[BOARD_INFO] device_name CYSBSYSKIT-DEV-01")
#define BOARD_INFO_DEVICE_VERSION      ("[BOARD_INFO] device_version 1.0.0")


#define CONFIG                         ("[CONFIG]")
#define CONFIG_MODE                    ("[CONFIG] mode ")
#define CONFIG_MAX_RANGE               ("[CONFIG] max_range ")
#define CONFIG_MIN_RANGE               ("[CONFIG] min_range ")
#define CONFIG_MACRO_THRESHOLD         ("[CONFIG] macro_threshold ")
#define CONFIG_MICRO_THRESHOLD         ("[CONFIG] micro_threshold ")
#define CONFIG_BANDPASS_FILTER         ("[CONFIG] bandpass_filter ")
#define CONFIG_DECIMATION_FILTER       ("[CONFIG] decimation_filter ")


#define MSG                            ("[MSG]")
#define MSG_TYPE_ERROR                 ("[MSG] ERROR ")




#endif /* CLI_TASK_H_ */
