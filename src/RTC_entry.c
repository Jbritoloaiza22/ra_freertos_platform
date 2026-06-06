#include "RTC.h"

#include "FreeRTOS.h"

#include "task.h"

#include "semphr.h"

#include <string.h>

 

/* =========================

 * FSP HANDLES

 * ========================= */

extern rtc_instance_ctrl_t g_rtc0_ctrl;

extern const rtc_cfg_t     g_rtc0_cfg;

 

/* =========================

 * SHARED STATE

 * ========================= */

static SemaphoreHandle_t g_rtc_mutex = NULL;

static StaticSemaphore_t g_rtc_mutex_buf;

static rtc_time_t        g_current_time;

 

#define RTC_MUTEX_WAIT_MS   (10U)

#define RTC_REFRESH_MS      (500U)

 

/* =========================

 * INIT

 * ========================= */

void RTC_init(void)

{

    if (NULL == g_rtc_mutex)

    {

        g_rtc_mutex = xSemaphoreCreateMutexStatic(&g_rtc_mutex_buf);

    }

 

    (void) R_RTC_Open(&g_rtc0_ctrl, &g_rtc0_cfg);

 

    /* Seed a default date: 2026-06-05 12:00:00 (Fri) */

    rtc_time_t t;

    memset(&t, 0, sizeof(t));

    t.tm_year = 126;   /* years since 1900 -> 2026 */

    t.tm_mon  = 5;     /* 0..11 -> June           */

    t.tm_mday = 5;

    t.tm_hour = 12;

    t.tm_min  = 0;

    t.tm_sec  = 0;

    t.tm_wday = 5;     /* Friday */

 

    (void) R_RTC_CalendarTimeSet(&g_rtc0_ctrl, &t);

 

    if (xSemaphoreTake(g_rtc_mutex, portMAX_DELAY) == pdTRUE)

    {

        g_current_time = t;

        xSemaphoreGive(g_rtc_mutex);

    }

}

 

/* =========================

 * GETTER (used by GUI task)

 * ========================= */

bool RTC_GetTime(rtc_time_t * out)

{

    if ((NULL == out) || (NULL == g_rtc_mutex))

    {

        return false;

    }

 

    if (xSemaphoreTake(g_rtc_mutex, pdMS_TO_TICKS(RTC_MUTEX_WAIT_MS)) != pdTRUE)

    {

        return false;

    }

 

    *out = g_current_time;

    xSemaphoreGive(g_rtc_mutex);

    return true;

}

 

/* =========================

 * TASK ENTRY

 * ========================= */

void RTC_entry(void * pvParameters)

{

    FSP_PARAMETER_NOT_USED(pvParameters);

 

    RTC_init();

 

    rtc_time_t t;

    for (;;)

    {

        if (R_RTC_CalendarTimeGet(&g_rtc0_ctrl, &t) == FSP_SUCCESS)

        {

            if (xSemaphoreTake(g_rtc_mutex, pdMS_TO_TICKS(RTC_MUTEX_WAIT_MS)) == pdTRUE)

            {

                g_current_time = t;

                xSemaphoreGive(g_rtc_mutex);

            }

        }

        vTaskDelay(pdMS_TO_TICKS(RTC_REFRESH_MS));

    }

}