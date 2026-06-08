#include "st7789.h"
#include "hal_data.h"
#include "bsp_api.h"
#include "r_ioport.h"
#include "r_spi.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>
#include <string.h>

/* =========================
 * DISPLAY CONFIG
 * ========================= */
#define LCD_H_RES 240
#define LCD_V_RES 320

#define LCD_BUF_LINES 10
/* =========================
 * FSP HANDLES
 * ========================= */
extern spi_instance_ctrl_t g_spi0_ctrl;
extern const spi_cfg_t g_spi0_cfg;

extern ioport_instance_ctrl_t g_ioport_ctrl;

/* =========================
 * PINS
 * ========================= */
#define PIN_DC   BSP_IO_PORT_01_PIN_07
#define PIN_RST  BSP_IO_PORT_01_PIN_15
#define PIN_BL   BSP_IO_PORT_01_PIN_05
#define PIN_CS   BSP_IO_PORT_01_PIN_12

/* =========================
 * LVGL
 * ========================= */
static lv_display_t *disp;
static uint16_t draw_buf[LCD_H_RES * LCD_BUF_LINES];

/* SPI sync (binary semaphore signaled from spi_callback)*/

static TaskHandle_t spi_owner_task;

void spi_callback(spi_callback_args_t *p_args)
{
    if(NULL == p_args)
    {
        return;
    }

    if(SPI_EVENT_TRANSFER_COMPLETE == p_args->event)
    {
        BaseType_t hp_woken = pdFALSE;
        if(NULL != spi_owner_task)
        {
            BaseType_t hp_woken = pdFALSE;
            vTaskNotifyGiveFromISR(spi_owner_task, &hp_woken);
            portYIELD_FROM_ISR(hp_woken);
        }
    }
}


/* =========================
 * GPIO helper
 * ========================= */
static inline void pin_write(bsp_io_port_pin_t pin, bool level)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl,
                      pin,
                      level ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

/* =========================
 * SPI WRITE (FSP CORRECTO)
 * ========================= */
static void st7789_write(const uint8_t *data, uint32_t len)
{
    spi_owner_task = xTaskGetCurrentTaskHandle();

    if(0 ==len)
    {
        return;
    }
    (void)ulTaskNotifyTake(pdTRUE,0);
    pin_write(PIN_CS, false);
    fsp_err_t err = R_SPI_Write(&g_spi0_ctrl,
                      data,
                      len,
                      SPI_BIT_WIDTH_8_BITS);
    if (FSP_SUCCESS != err)
    {
        /* aquí podrías loggear error */
        pin_write(PIN_CS, true);
        return;
    }
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    pin_write(PIN_CS, true);
}

/* =========================
 * CMD / DATA
 * ========================= */
static void st7789_cmd(uint8_t cmd)
{
    pin_write(PIN_DC, false);
    st7789_write(&cmd, 1);
}

static void st7789_data(const uint8_t *data, uint32_t len)
{
    pin_write(PIN_DC, true);
    st7789_write(data, len);
}

/* =========================
 * HW INIT
 * ========================= */
static void st7789_hw_init(void)
{
    (void) R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_CS, IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_HIGH);
    (void) R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_DC, IOPORT_CFG_PORT_DIRECTION_OUTPUT );
    (void) R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_RST, IOPORT_CFG_PORT_DIRECTION_OUTPUT );
    (void) R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_BL, IOPORT_CFG_PORT_DIRECTION_OUTPUT );
    pin_write(PIN_CS, true);
    (void) R_SPI_Open(&g_spi0_ctrl, &g_spi0_cfg);
    pin_write(PIN_BL, true);

    pin_write(PIN_RST, false);
    R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MILLISECONDS);
    pin_write(PIN_RST, true);
    R_BSP_SoftwareDelay(120, BSP_DELAY_UNITS_MILLISECONDS);

    st7789_cmd(0x11); // sleep out
    R_BSP_SoftwareDelay(120, BSP_DELAY_UNITS_MILLISECONDS);

    uint8_t colmod = 0x55; // RGB565
    st7789_cmd(0x3A);
    st7789_data(&colmod, 1);

    uint8_t madcl = 0x00;
    st7789_cmd(0x36);
    st7789_data(&madcl, 1);

    st7789_cmd(0x21); // inversion on

    st7789_cmd(0x29); // display on
}

/* =========================
 * WINDOW
 * ========================= */
static void st7789_set_window(int x1, int y1, int x2, int y2)
{
    uint8_t data[4];

    st7789_cmd(0x2A);
    data[0] = x1 >> 8;
    data[1] = x1 & 0xFF;
    data[2] = x2 >> 8;
    data[3] = x2 & 0xFF;
    st7789_data(data, 4);

    st7789_cmd(0x2B);
    data[0] = y1 >> 8;
    data[1] = y1 & 0xFF;
    data[2] = y2 >> 8;
    data[3] = y2 & 0xFF;
    st7789_data(data, 4);

    st7789_cmd(0x2C);
}

/* =========================
 * LVGL FLUSH CALLBACK
 * ========================= */
static void lvgl_flush_cb(lv_display_t *d,
                          const lv_area_t *area,
                          uint8_t *px_map)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    st7789_set_window(area->x1, area->y1,
                      area->x2, area->y2);

    pin_write(PIN_DC, true);

    /* enviar buffer completo */
    st7789_write(px_map, w * h * 2);

    lv_display_flush_ready(d);
}

/* =========================
 * INIT PUBLICO
 * ========================= */
void st7789_init(void)
{
    /* HW init */
    st7789_hw_init();

    /* LVGL display create */
    disp = lv_display_create(LCD_H_RES, LCD_V_RES);

    if(NULL == disp)
    {
        /* aquí podrías loggear error */
        return;
    }

    lv_display_set_buffers(disp,
                          draw_buf,
                          NULL,
                          sizeof(draw_buf),
                          LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_flush_cb(disp, lvgl_flush_cb);
}
