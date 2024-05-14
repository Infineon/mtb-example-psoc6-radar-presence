/*****************************************************************************
 * File name: radar_config_optimizer.h
 *
 * Description: This file contains types and function prototypes
 *
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

#ifndef SOURCE_RADAR_CONFIG_OPTIMIZER_H_
#define SOURCE_RADAR_CONFIG_OPTIMIZER_H_

#include <inttypes.h>
#include <stdio.h>

#include "xensiv_radar_presence.h"

/*
 * @def enum optimization_type_e
 * Type of optimization
 * CONFIG_LOW_FRAME_RATE_OPT - radar is working with low frame rate,
 * CONFIG_HIGH_FRAME_RATE_OPT - radar is working with high frame rate
 * CONFIG_UNINITIALIZED - initial value
 */
typedef enum
{
    CONFIG_LOW_FRAME_RATE_OPT,
    CONFIG_HIGH_FRAME_RATE_OPT,
    CONFIG_UNINITIALIZED = 64
} optimization_type_e;

/*
 * @def enum radar_configurator_status_e
 * Status of radar configuration's functions
 * ESTATUS_SUCCESS - success
 * ESTATUS_FAILURE - fail
 * ESTATUS_INVAL_PARAM_VAL - bad param
 * ESTATUS_PARM_NOT_SUPPORTED - parameter not supported
 * ESTATUS_PARAM_UNINITIALIZED - parameter uninitialized
 */
typedef enum
{
    ESTATUS_SUCCESS,
    ESTATUS_FAILURE,
    ESTATUS_INVAL_PARAM_VAL,
    ESTATUS_PARM_NOT_SUPPORTED,
    ESTATUS_PARAM_UNINITIALIZED
} radar_configurator_status_e;

/*
 * @typedef typedef void (*configure_radar_sensor)(optimization_type_e)
 * Configure radar sensor callback prototype. The configure radar sensor's callback function must follow this prototype.
 */
typedef void (*configure_radar_sensor)(optimization_type_e);


/*******************************************************************************
 * Function Name: radar_config_optimizer_init
 ****************************************************************************//**
 *
 * @brief Initializes the radar configuration optimizer by setting the
 * function pointer for reconfiguring the radar sensor.
 *
 * @param pfn_reconf_radar Pointer to the function that will be called to
 * reconfigure the radar sensor.
 *
 * @return radar_configurator_status_e Returns the status of the initialization.
 * ESTATUS_SUCCESS if successful,
 * ESTATUS_PARAM_UNINITIALIZED if the input parameter is uninitialized.
 *
 *******************************************************************************/
radar_configurator_status_e radar_config_optimizer_init(configure_radar_sensor pfn_reconf_radar);

/*******************************************************************************
 * Function Name: radar_config_optimizer_set_operational_mode
 ****************************************************************************//**
 *
 * @brief Sets the operational mode for the radar configuration optimizer.
 *
 * @param user_mode The selected mode for the radar configuration optimizer.
 *
 * @return radar_configurator_status_e Returns the status of the operation.
 * ESTATUS_SUCCESS if successful,
 * ESTATUS_PARM_NOT_SUPPORTED if the input parameter is not supported.
 *
 *******************************************************************************/
radar_configurator_status_e radar_config_optimizer_set_operational_mode(xensiv_radar_presence_mode_t user_mode);

/*******************************************************************************
 * Function Name: radar_config_optimize
 ****************************************************************************//**
 *
 * @brief Optimizes the radar configuration based on the current presence state.
 * This function optimizes the radar configuration based on the current
 * presence state. The radar configuration optimizer must be initialized with
 * a valid configuration function using radar_config_optimizer_init()
 * before calling this function.
 *
 * @param current_state The current radar presence state.
 *
 * @return radar_configurator_status_e Returns the status of the operation.
 * ESTATUS_SUCCESS if successful,
 * ESTATUS_FAILURE if the radar configuration optimizer is not initialized.
 *
 *******************************************************************************/
radar_configurator_status_e radar_config_optimize(xensiv_radar_presence_state_t current_state);

/*******************************************************************************
 * Function Name: radar_config_get_current_optimization
 ****************************************************************************//**
 *
 * @brief Returns radar current config
 *
 * @param void
 *
 * @return radar_configurator_status_e Returns radar current config
 *
 *******************************************************************************/
optimization_type_e radar_config_get_current_optimization(void);

#endif /* SOURCE_RADAR_CONFIG_OPTIMIZER_H_ */
