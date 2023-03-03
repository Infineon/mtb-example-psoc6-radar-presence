/******************************************************************************
* File Name: resource_map.h
*
* Description: This file defines the SPI and GPIO pin map for all the supported kits.
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

#ifndef RESOURCE_MAP_H_
#define RESOURCE_MAP_H_
#include "cybsp.h"

#ifdef TARGET_APP_CYSBSYSKIT_DEV_01

#define LED_RGB_RED                         CYBSP_GPIOA0
#define LED_RGB_GREEN                       CYBSP_GPIOA1
#define LED_RGB_BLUE                        CYBSP_GPIOA2

#define PIN_XENSIV_BGT60TRXX_SPI_SCLK       CYBSP_SPI_CLK
#define PIN_XENSIV_BGT60TRXX_SPI_MOSI       CYBSP_SPI_MOSI
#define PIN_XENSIV_BGT60TRXX_SPI_MISO       CYBSP_SPI_MISO
#define PIN_XENSIV_BGT60TRXX_SPI_CSN        CYBSP_SPI_CS
#define PIN_XENSIV_BGT60TRXX_IRQ            CYBSP_GPIO10
#define PIN_XENSIV_BGT60TRXX_RSTN           CYBSP_GPIO11
#define PIN_XENSIV_BGT60TRXX_LDO_EN         CYBSP_GPIO5

#endif

#ifdef TARGET_APP_KIT_BGT60TR13C_EMBEDD

#define LED_RGB_RED                         CYBSP_LED_RGB_RED
#define LED_RGB_GREEN                       CYBSP_LED_RGB_GREEN
#define LED_RGB_BLUE                        CYBSP_LED_RGB_BLUE

#define PIN_XENSIV_BGT60TRXX_SPI_SCLK       CYBSP_RADAR_SPI_CLK
#define PIN_XENSIV_BGT60TRXX_SPI_MOSI       CYBSP_RADAR_SPI_MOSI
#define PIN_XENSIV_BGT60TRXX_SPI_MISO       CYBSP_RADAR_SPI_MISO
#define PIN_XENSIV_BGT60TRXX_SPI_CSN        CYBSP_RADAR_SPI_CS
#define PIN_XENSIV_BGT60TRXX_IRQ            CYBSP_RADAR_IRQ
#define PIN_XENSIV_BGT60TRXX_RSTN           CYBSP_RADAR_RST
#define PIN_XENSIV_BGT60TRXX_LDO_EN         CYBSP_RADAR_EN_LDO

#endif

#endif 
