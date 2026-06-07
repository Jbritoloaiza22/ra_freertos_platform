#include "ANOMALY.h"
/* ANOMALY_task entry function */
/* pvParameters contains TaskHandle_t */
void ANOMALY_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */
    while (1)
    {
        vTaskDelay (1);
    }
}
