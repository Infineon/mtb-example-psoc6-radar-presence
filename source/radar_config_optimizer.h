/*****************************************************************************
 * File name: radar_config_optimizer.h
 *
 * Description: This file contains types and function prototypes
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
