#include "hal_data.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lvgl.h"
#include "st7789.h"
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

    label = lv_label_create(scr);
    lv_label_set_text(label, "POWER:      0 W");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

    label = lv_label_create(scr);
    lv_label_set_text(label, "VOLTAGE:  0.0 V");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 25);

    label = lv_label_create(scr);
    lv_label_set_text(label, "CURRENT:  0.0 A");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 50);

    label = lv_label_create(scr);
    lv_label_set_text(label, "TEMP:     0.0 C");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 75);

    label = lv_label_create(scr);
    lv_label_set_text(label, "TIME: 00:00:00");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 100);


    /* -------------------------
     * Main LVGL loop
     * ------------------------- */
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
