#pragma once

#include "hal_data.h"
#include "bsp_api.h"
#include <stdbool.h>
#include <stdint.h>

#define RELAY_BTN_PIN (BSP_IO_PORT_02_PIN_06)
#define RELAY_OUT_PIN (BSP_IO_PORT_04_PIN_10)

void RELAY_entry(void * pvParameters);