#include "RELAY.h"

 

#include "FreeRTOS.h"

#include "task.h"

 

/* Poll period and simple debounce window (in ticks). */

#define RELAY_POLL_TICKS        (pdMS_TO_TICKS(10U))

#define RELAY_DEBOUNCE_SAMPLES  (3U)

 

/* RELAY_interface entry function */

/* pvParameters contains TaskHandle_t */

void RELAY_entry(void *pvParameters)

{

    FSP_PARAMETER_NOT_USED (pvParameters);

 

    /* Button is active LOW. Start assuming released (HIGH). */

    bsp_io_level_t stable_btn   = BSP_IO_LEVEL_HIGH;

    bsp_io_level_t last_sample  = BSP_IO_LEVEL_HIGH;

    uint32_t       stable_count = 0U;

 

    /* Latched output state, toggled on each press. */

    bsp_io_level_t out_state    = BSP_IO_LEVEL_LOW;

 

    /* Ensure relay output starts OFF. */

    R_BSP_PinAccessEnable();

    R_BSP_PinWrite(RELAY_OUT_PIN, out_state);

    R_BSP_PinAccessDisable();

 

    while (1)

    {

        R_BSP_PinAccessEnable();

        bsp_io_level_t sample = (bsp_io_level_t) R_BSP_PinRead(RELAY_BTN_PIN);

        R_BSP_PinAccessDisable();

 

        /* Debounce: count consecutive identical samples before accepting. */

        if (sample == last_sample)

        {

            if (stable_count < RELAY_DEBOUNCE_SAMPLES)

            {

                stable_count++;

            }

        }

        else

        {

            stable_count = 1U;

            last_sample  = sample;

        }

 

        /* When the debounced level changes, look for press edge (HIGH -> LOW). */

        if ((stable_count >= RELAY_DEBOUNCE_SAMPLES) && (sample != stable_btn))

        {

            stable_btn = sample;

 

            if (BSP_IO_LEVEL_LOW == stable_btn)

            {

                /* Press detected: toggle output. */

                out_state = (BSP_IO_LEVEL_LOW == out_state)

                            ? BSP_IO_LEVEL_HIGH

                            : BSP_IO_LEVEL_LOW;

 

                R_BSP_PinAccessEnable();

                R_BSP_PinWrite(RELAY_OUT_PIN, out_state);

                R_BSP_PinAccessDisable();

            }

        }

 

        vTaskDelay(RELAY_POLL_TICKS);

    }

}