#include "hal_data.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lvgl.h"
#include "st7789.h"
#include "RTC.h"
#include "TEMP.h"
#include "RELAY.h"
#include <stdio.h>
#include <stdbool.h>
#include "PZEM.h"

#define GUI_TICK_MS (5U)
#define TIME_REFRESH_MS (1000U)
#define TEMP_REFRESH_MS (1000U)
#define PZEM_REFRESH_MS (200U)
 

 bool TEMP_GetCelsius(float *out_c);
void TEMP_entry(void *pvParameters);
static uint32_t lv_tick_get_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}
/* FSP GUI thread entry */
void GUI_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    lv_init();
    lv_tick_set_cb(lv_tick_get_ms);

    /* -------------------------
     * Init hardware / drivers
     * ------------------------- */
    st7789_init();

    /* -------------------------
     * Simple test UI
     * ------------------------- */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x4169E1),LV_PART_MAIN);

    lv_obj_t *label;

    lv_obj_t *power_label = lv_label_create(scr);
    lv_label_set_text(power_label, "POWER:      0 W");
    lv_obj_set_style_text_color(power_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(power_label, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *volt_label = lv_label_create(scr);
    lv_label_set_text(volt_label, "VOLTAGE:  0.0 V");
    lv_obj_set_style_text_color(volt_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(volt_label, LV_ALIGN_TOP_LEFT, 0, 25);

    lv_obj_t *curr_label = lv_label_create(scr);
    lv_label_set_text(curr_label, "CURRENT:  0.0 A");
    lv_obj_set_style_text_color(curr_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(curr_label, LV_ALIGN_TOP_LEFT, 0, 50);
    (void) label;

    lv_obj_t *temp_label = lv_label_create(scr);
    lv_label_set_text(temp_label, "TEMP:   --.- C");
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 0, 75);

    lv_obj_t *time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "TIME: --:--:--");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 0, 100);


    /* -------------------------
     * Main LVGL loop
     * ------------------------- */
    uint32_t time_acc_ms = 0;
    uint32_t temp_acc_ms = 0;
    uint32_t pzem_acc_ms = 0;
    rtc_time_t rtc_now;
    float temp_c = 0.0f;
    pzem_data_t pzem;

    char time_buf[32];
    char temp_buf[32];
    char pzem_buf[32];

    while (1)
    {
        lv_timer_handler();

        time_acc_ms += GUI_TICK_MS;
        if (time_acc_ms >= TIME_REFRESH_MS)
        {
            time_acc_ms = 0;

                if (RTC_GetTime(&rtc_now))
                {
                    snprintf(time_buf, sizeof(time_buf), "TIME: %02d:%02d:%02d",
                             rtc_now.tm_hour, rtc_now.tm_min, rtc_now.tm_sec);
                    lv_label_set_text(time_label, time_buf);
                }
        }

        temp_acc_ms += GUI_TICK_MS;
        if (temp_acc_ms >= TEMP_REFRESH_MS)
        {
            temp_acc_ms = 0;

            if (TEMP_GetCelsius(&temp_c))
            {
                snprintf(temp_buf, sizeof(temp_buf), "TEMP: %5.1f C", (double)temp_c);
                lv_label_set_text(temp_label, temp_buf);
            }
        }

        pzem_acc_ms += GUI_TICK_MS;
        if (pzem_acc_ms >= PZEM_REFRESH_MS)
        {
            pzem_acc_ms = 0;

            if (PZEM_GetData(&pzem))
            {
                snprintf(pzem_buf, sizeof(pzem_buf), "POWER: %6.1f W",
                (double) pzem.power_w);
                lv_label_set_text(power_label, pzem_buf);
                snprintf(pzem_buf, sizeof(pzem_buf), "VOLTAGE: %5.1f V",
                (double) pzem.voltage_v);
                lv_label_set_text(volt_label, pzem_buf);
                snprintf(pzem_buf, sizeof(pzem_buf), "CURRENT: %5.3f A",
                (double) pzem.current_a);
                lv_label_set_text(curr_label, pzem_buf);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(GUI_TICK_MS));
    }
}
