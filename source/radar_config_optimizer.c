/*****************************************************************************
 * File name: radar_config_optimizer.c
 *
 * Description: This file implements radar configuration on the fly
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

#include "radar_config_optimizer.h"

#define DEBUG_RECONFIG

extern bool print_job_locked;

typedef struct
{
    xensiv_radar_presence_mode_t selected_mode;
    optimization_type_e current_optimization;
    configure_radar_sensor pfn_reconf_radar;
} radar_config_optimizer_s;

static radar_config_optimizer_s optimizer_state =
{
        .selected_mode        = XENSIV_RADAR_PRESENCE_MODE_MACRO_ONLY,
        .current_optimization = CONFIG_UNINITIALIZED,
        .pfn_reconf_radar     = NULL
};

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
radar_configurator_status_e radar_config_optimizer_init(configure_radar_sensor pfn_reconf_radar)
{
    radar_configurator_status_e result = ESTATUS_FAILURE;

    if (pfn_reconf_radar == NULL)
    {
        result = ESTATUS_PARAM_UNINITIALIZED;
    }
    else
    {
        optimizer_state.pfn_reconf_radar = pfn_reconf_radar;
        result = ESTATUS_SUCCESS;
    }
    
    return result;
}

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
radar_configurator_status_e radar_config_optimizer_set_operational_mode(
        xensiv_radar_presence_mode_t user_mode)
{
    radar_configurator_status_e result = ESTATUS_FAILURE;

    if ((user_mode == XENSIV_RADAR_PRESENCE_MODE_MACRO_ONLY) ||
            (user_mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_ONLY)
            || (user_mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO) ||
            (user_mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_AND_MACRO))
    {
        optimizer_state.selected_mode = user_mode;
        if (optimizer_state.current_optimization == CONFIG_UNINITIALIZED)
        {
            if ((user_mode == XENSIV_RADAR_PRESENCE_MODE_MACRO_ONLY) ||
                    (user_mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO))
            {
                optimizer_state.current_optimization = CONFIG_LOW_FRAME_RATE_OPT;
            }
            else /* micro_only or ( micro & macro ) */
            {
                optimizer_state.current_optimization = CONFIG_HIGH_FRAME_RATE_OPT;
            }
        }

        result =  ESTATUS_SUCCESS;
    }
    else
    {
        result = ESTATUS_PARM_NOT_SUPPORTED;
    }

    return result;
}

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
radar_configurator_status_e radar_config_optimize(xensiv_radar_presence_state_t current_state)
{
    radar_configurator_status_e result = ESTATUS_FAILURE;

    if (optimizer_state.pfn_reconf_radar == NULL)
    {
        result = ESTATUS_FAILURE;
    }
    else
    {
        optimization_type_e req_optimization = CONFIG_UNINITIALIZED;

    #ifdef DEBUG_RECONFIG
      if(!print_job_locked)
      { 
           printf("current presence: %d, current setting: %s\n", current_state,
                optimizer_state.current_optimization==CONFIG_LOW_FRAME_RATE_OPT?"10 Hz":"200 Hz");
      }
    #endif

        if (optimizer_state.selected_mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO)
        {
            switch (current_state)
            {
            case XENSIV_RADAR_PRESENCE_STATE_MACRO_PRESENCE:
                if (optimizer_state.current_optimization == CONFIG_LOW_FRAME_RATE_OPT)
                {
                    req_optimization = CONFIG_HIGH_FRAME_RATE_OPT;
                }
                else if (optimizer_state.current_optimization == CONFIG_HIGH_FRAME_RATE_OPT)
                {
                    req_optimization = CONFIG_LOW_FRAME_RATE_OPT;
                }
                break;

            case XENSIV_RADAR_PRESENCE_STATE_ABSENCE:
                    
                req_optimization = CONFIG_LOW_FRAME_RATE_OPT;

                break;

            default:
                req_optimization = optimizer_state.current_optimization;
                break;
            }
        }
        else if ((optimizer_state.selected_mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_ONLY) ||
                (optimizer_state.selected_mode == XENSIV_RADAR_PRESENCE_MODE_MICRO_AND_MACRO))
        {
            req_optimization = CONFIG_HIGH_FRAME_RATE_OPT;
        }
        else if (optimizer_state.selected_mode == XENSIV_RADAR_PRESENCE_MODE_MACRO_ONLY)
        {
            req_optimization = CONFIG_LOW_FRAME_RATE_OPT;
        }

        if (req_optimization != optimizer_state.current_optimization)
        {
            optimizer_state.current_optimization = req_optimization;
            optimizer_state.pfn_reconf_radar(req_optimization);

#ifdef DEBUG_RECONFIG
        if (!print_job_locked)
        {

            printf("new setting: %s\n", optimizer_state.current_optimization == CONFIG_LOW_FRAME_RATE_OPT ? "macro" : "micro");
        }
#endif
        }

        result = ESTATUS_SUCCESS;
    }

    return result;
}

/*******************************************************************************
 * Function Name: radar_config_get_current_optimization
 ****************************************************************************//**
 *
 * @brief Get the current optimization type that the radar optimizer is using.
 *
 * @return The current optimization type that the radar optimizer is using.
 *
 *******************************************************************************/
optimization_type_e radar_config_get_current_optimization(void)
{
    return optimizer_state.current_optimization;
}
