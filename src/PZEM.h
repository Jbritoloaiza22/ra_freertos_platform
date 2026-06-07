#pragma once
#include "hal_data.h"
#include "bsp_api.h"
#include "r_sci_uart.h"
#include "r_uart_api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* =========================================================================
 * PZEM-004T (v3.0) AC energy meter driver - Modbus RTU over UART
 *  - 9600 baud, 8N1
 *  - Default address 0xF8 (general/single-slave)
 *  - Read 10 input registers from 0x0000 (function 0x04)
 *  - CRC-16 Modbus, poly 0xA001, init 0xFFFF, wire order LO then HI
 * ========================================================================= */
#define PZEM_ADDR_GENERAL        (0xF8U)  /* single-slave / general address */
#define PZEM_ADDR_BROADCAST      (0x00U)  /* slave does NOT reply           */
/* Latest measurement snapshot. */
typedef struct
{
    float    voltage_v;     /* 0.0 .. 260.0 V        */
    float    current_a;     /* 0.000 .. 100.000 A    */
    float    power_w;       /* 0.0 .. 23000.0 W      */
    uint32_t energy_wh;     /* 0 .. 9 999 990 Wh     */
    float    frequency_hz;  /* 45.0 .. 65.0 Hz       */
    float    power_factor;  /* 0.00 .. 1.00          */
    bool     alarm;         /* true if over-power    */
} pzem_data_t;
/* FreeRTOS task entry: opens UART0, polls the meter once per second and
 * publishes the latest reading to an internal length-1 mailbox queue. */
void PZEM_entry(void * pvParameters);
/* Peek the latest reading without consuming it.
 * Returns true if a sample is available. */
bool PZEM_GetData(pzem_data_t * out);
/* One-shot transaction helpers. They block while running and must only be
 * called after PZEM_entry() has opened the UART. */
bool PZEM_ResetEnergy(uint8_t addr);
bool PZEM_SetAlarmThreshold(uint8_t addr, uint16_t watts);
bool PZEM_SetAddress(uint8_t current_addr, uint8_t new_addr);