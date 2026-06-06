#include "TEMP.h"
/* TEMP_interface entry function */
/* pvParameters contains TaskHandle_t */
 uint16_t g_adc_val;
 volatile bool interrupt_called = false;
 adc_event_t adc_event;
 uint16_t adc_data_vref;
 float temp_c = 0;
void TEMP_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    fsp_err_t err;
    adc_status_t adc_status;


    uint32_t cal_127;
    /* TODO: add your own code here */

    err = R_ADC_Open(&g_adc0_ctrl,&g_adc0_cfg);
    assert(FSP_SUCCESS == err);
    err = R_ADC_ScanCfg(&g_adc0_ctrl, &g_adc0_channel_cfg);
    assert(FSP_SUCCESS == err);

    while (1)
    {
        err = R_ADC_ScanStart(&g_adc0_ctrl);
        assert(FSP_SUCCESS == err);
        while(!interrupt_called);
        assert(adc_event == ADC_EVENT_SCAN_COMPLETE);
        err = R_ADC_Read(&g_adc0_ctrl, ADC_CHANNEL_TEMPERATURE, &g_adc_val);
        assert(FSP_SUCCESS == err);
         temp_c = 26.0f +
                       ((33119.0f - (float)g_adc_val) / 700.0f);
        err = R_ADC_Read(&g_adc0_ctrl, ADC_CHANNEL_VOLT, &adc_data_vref);
        vTaskDelay (1);
    }
}
/* Callback function */
void g_adc_callback(adc_callback_args_t *p_args)
{
    /* TODO: add your own code here */
    interrupt_called = true;
    adc_event = p_args->event;
}


