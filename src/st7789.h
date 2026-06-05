/******************************************************************************
 * @file    st7789.h
 * @brief   Public interface for the ST7789 LCD Driver
 *
 * @author  Jesus Daniel Britoloaiza
 *
 * @copyright
 * Copyright (c) 2026 Jesus Daniel Britoloaiza.
 *
 * This file is part of an educational and academic project developed for
 * learning purposes in embedded systems, ESP32 programming, FreeRTOS,
 * display drivers, and LVGL graphical user interfaces.
 *
 * Permission is granted to use, study, modify, and distribute this source
 * code for educational and non-commercial purposes provided that the author
 * attribution and this notice are preserved.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND.
 *
 ******************************************************************************/

#pragma once


#include "hal_data.h"
#include "lvgl.h"

/**
 * @brief Initialize ST7789 + LVGL display
 */
void st7789_init(void);
