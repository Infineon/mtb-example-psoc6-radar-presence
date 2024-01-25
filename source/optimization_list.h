/*****************************************************************************
 * File name: optimization_list.h
 *
 * Description: This file contains list of available configurations
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
