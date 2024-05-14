/*****************************************************************************
 * File name: optimization_list.h
 *
 * Description: This file contains list of available configurations
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

#ifndef SOURCE_OPTIMIZATION_LIST_H_
#define SOURCE_OPTIMIZATION_LIST_H_

#include "radar_low_framerate_config.h"
#include "radar_high_framerate_config.h"

/*
 * @def NUM_SAMPLES_PER_FRAME
 * Number of samples per frame
 * @note: Number of samples per frame
 */
#define NUM_SAMPLES_PER_FRAME               (XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP *\
                                             XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME *\
                                             XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS)

/*
 * @def NUM_CHIRPS_PER_FRAME
 * Number of chirps per frame
 * @note: Number of chirps per frame
 */
#define NUM_CHIRPS_PER_FRAME                XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME

/*
 * @def NUM_SAMPLES_PER_CHIRP
 * Number of samples per chirp
 * @note: Number of samples per chirp
 */
#define NUM_SAMPLES_PER_CHIRP               XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP

/*
 * @def MACRO_FFT_BUFF_SIZE
 * Size of Macro FFT buffer
 * @note: Size of Macro FFT buffer
 */
#define MACRO_FFT_BUFF_SIZE                 30U  //size of macro fft buffer for presence application

/*
 * @typedef typedef struct  optimization_s
 * Optimization interface .
 */
typedef struct {

    uint32_t *reg_list;
    uint8_t  reg_list_size;
    uint32_t fifo_limit;
}optimization_s;

optimization_s optimizations_list [] = {
        {
                register_list_macro_only,
                XENSIV_BGT60TRXX_CONF_NUM_REGS_MACRO,
                NUM_SAMPLES_PER_FRAME*2
        },
        {
                register_list_micro_only,
                XENSIV_BGT60TRXX_CONF_NUM_REGS_MICRO,
                NUM_SAMPLES_PER_FRAME*2
        }
};

#endif /* SOURCE_OPTIMIZATION_LIST_H_ */
