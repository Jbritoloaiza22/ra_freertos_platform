#pragma once

#include "hal_data.h"
#include "bsp_api.h"
#include "PZEM.h"
#include <stdbool.h>
#include <stdint.h>

/* =========================================================================
 * Anomaly detector for PZEM measurements.
 *
 * Today: rule-based + z-score on voltage.
 * Future: replace anomaly_score() with a TFLite Micro / CMSIS-NN call.
 *         The feature vector layout is stable so consumers do not change.
 * ========================================================================= */

/* Bit flags. OR-combined into pzem_anomaly_t.flags. */
typedef enum
{
    ANOMALY_NONE          = 0U,
    ANOMALY_SIGNAL_LOSS   = (1U << 0),  /* No fresh sample from meter      */
    ANOMALY_OVER_VOLTAGE  = (1U << 1),  /* V > V_MAX                       */
    ANOMALY_UNDER_VOLTAGE = (1U << 2),  /* V < V_MIN (and signal present)  */
    ANOMALY_FREQ_OUT      = (1U << 3),  /* F outside [F_MIN, F_MAX]        */
    ANOMALY_PF_LOW        = (1U << 4),  /* Low power factor with load      */
    ANOMALY_SUDDEN_DELTA  = (1U << 5),  /* |z-score| over threshold        */
    ANOMALY_STUCK         = (1U << 6),  /* Sensor frozen (no variation)    */
} anomaly_flag_t;
#define ANOMALY_FEATURE_COUNT (6U)
/* Aggregated severity for UI / supervisory logic. */
typedef enum
{
    ANOMALY_STATE_NORMAL  = 0,  /* nothing tripped, score low      */
    ANOMALY_STATE_WARNING = 1,  /* soft anomaly (low PF, mild z)   */
    ANOMALY_STATE_FAULT   = 2,  /* hard anomaly (signal loss, OV)  */
} anomaly_state_t;

typedef struct
{
    uint32_t        flags;                              /* OR of anomaly_flag_t */
    anomaly_state_t state;                              /* aggregated severity  */
    float           score;                              /* [0.0, 1.0]           */
    uint32_t        tick_ms;                            /* Time of detection    */
    float           features[ANOMALY_FEATURE_COUNT];   /* V, I, P, F, PF, |z_V|*/
} pzem_anomaly_t;

/* Human-readable label ("NORMAL" / "WARNING" / "FAULT"). */
const char * ANOMALY_StateLabel(anomaly_state_t s);

/* Initialise the anomaly mailbox queue. Safe to call multiple times. */
void ANOMALY_QueueInit(void);

/* Peek the latest anomaly snapshot without consuming it.
 * Returns true if at least one cycle has been published. */
bool ANOMALY_GetLatest(pzem_anomaly_t * out);

/* FreeRTOS task entry. */
void ANOMALY_entry(void * pvParameters);