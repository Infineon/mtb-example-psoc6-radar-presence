/*****************************************************************************
 * File name: presence_settings.h
 *
 * Description: This file contains presence algorithm starting configuration
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

#ifndef XENSIV_RADAR_PRESENCE_SETTINGS_H
#define XENSIV_RADAR_PRESENCE_SETTINGS_H

#include "radar_low_framerate_config.h"
#include "xensiv_radar_presence.h"

#if defined(XENSIV_RADAR_PRESENCE_SETTINGS_H_IMPL)
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

#endif /*XENSIV_RADAR_PRESENCE_SETTINGS_H_IMPL*/
#endif /* XENSIV_RADAR_PRESENCE_SETTINGS_H*/
