#include "hal_data.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lvgl.h"
#include "st7789.h"

/* FSP GUI thread entry */
void GUI_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    /* -------------------------
     * Init hardware / drivers
     * ------------------------- */
    st7789_init();

    lv_init();

    /* -------------------------
     * Simple test UI
     * ------------------------- */
    //lv_obj_t *label = lv_label_create(lv_screen_active());
    //lv_label_set_text(label, "RA2A1 LVGL OK");
    //lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    /* -------------------------
     * Main LVGL loop
     * ------------------------- */
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
