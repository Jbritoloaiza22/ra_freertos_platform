#pragma once

#include "hal_data.h"
#include "bsp_api.h"
#include "r_rtc.h"
#include <stdbool.h>

void RTC_init(void);
bool RTC_GetTime(rtc_time_t *out);
void RTC_entry(void *pvParameters);
