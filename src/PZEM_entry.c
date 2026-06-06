#include "PZEM.h"

#include "FreeRTOS.h"

#include "task.h"

#include "semphr.h"

#include <string.h>

 

/* FSP-generated handles for UART0. */

extern sci_uart_instance_ctrl_t g_uart0_ctrl;

extern const uart_cfg_t         g_uart0_cfg;

 

/* Semaphore used to block uart_send_blocking until the TX_COMPLETE event

 * fires from the SCI ISR. */

static SemaphoreHandle_t g_uart_tx_sem  = NULL;

static StaticSemaphore_t g_uart_tx_sem_buf;

 

/* Optional: signal data RX events too, not used here but kept for symmetry. */

static volatile uart_event_t g_uart_last_event = (uart_event_t) 0;

 

/* UART callback: called from the SCI ISR. Must give the semaphore using

 * the FromISR variant. */

static void uart_callback(uart_callback_args_t * p_args)

{

    g_uart_last_event = p_args->event;

 

    if (UART_EVENT_TX_COMPLETE == p_args->event)

    {

        BaseType_t hp_woken = pdFALSE;

        xSemaphoreGiveFromISR(g_uart_tx_sem, &hp_woken);

        portYIELD_FROM_ISR(hp_woken);

    }

}

 

/* Send a NUL-terminated string and block until the bytes have been

 * shifted out of the UART (or the timeout expires). */

static bool uart_send_blocking(const char * s, uint32_t timeout_ms)

{

    if ((NULL == s) || (NULL == g_uart_tx_sem))

    {

        return false;

    }

 

    size_t len = strlen(s);

    if (0U == len)

    {

        return true;

    }

 

    /* Make sure the semaphore starts empty. */

    (void) xSemaphoreTake(g_uart_tx_sem, 0);

 

    if (FSP_SUCCESS != R_SCI_UART_Write(&g_uart0_ctrl,

                                        (uint8_t const *) s,

                                        len))

    {

        return false;

    }

 

    return pdTRUE == xSemaphoreTake(g_uart_tx_sem,

                                    pdMS_TO_TICKS(timeout_ms));

}

 

/* PZEM_Interface entry function */

/* pvParameters contains TaskHandle_t */

void PZEM_entry(void * pvParameters)

{

    FSP_PARAMETER_NOT_USED(pvParameters);

 

    g_uart_tx_sem = xSemaphoreCreateBinaryStatic(&g_uart_tx_sem_buf);

 

    if (FSP_SUCCESS != R_SCI_UART_Open(&g_uart0_ctrl, &g_uart0_cfg))

    {

        vTaskDelete(NULL);

        return;

    }

 

    /* Register our callback at runtime (configurator left it as NULL). */

    (void) R_SCI_UART_CallbackSet(&g_uart0_ctrl,

                                  uart_callback,

                                  NULL,

                                  NULL);

 

    while (1)

    {

        (void) uart_send_blocking("Hello World\r\n", 100U);

        vTaskDelay(pdMS_TO_TICKS(1000));

    }

}