#include "ANOMALY.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <math.h>
#include <string.h>
/* =========================================================================
 * Tunables.
 * ========================================================================= */
#define ANOMALY_PERIOD_MS         (1000U)  /* sampling cadence            */
#define ANOMALY_SIGNAL_TIMEOUT_MS (3500U)  /* > 3 missed polls = lost     */

/* Mains limits (Mexico 127 V / 60 Hz, with margin). */
#define V_MIN_VOLTS               (100.0f)
#define V_MAX_VOLTS               (140.0f)
#define F_MIN_HZ                  (58.0f)
#define F_MAX_HZ                  (62.0f)
/* Low PF only matters when there is real load. */
#define PF_LOAD_THRESHOLD_W       (10.0f)
#define PF_LOW_LIMIT              (0.60f)

/* z-score sudden-change detector. */
#define ZSCORE_TRIGGER            (4.0f)
#define ZSCORE_SCORE_DIVISOR      (10.0f)
#define WELFORD_WARMUP_SAMPLES    (8U)
#define VAR_FLOOR                 (0.25f)   /* avoids div-by-zero on flat V */

/* Stuck detector: N consecutive samples with |dV| < eps. */
#define STUCK_WINDOW              (6U)
#define STUCK_EPSILON_V           (0.05f)
#define SCORE_WARNING   (0.30f)
#define SCORE_FAULT     (0.70f)



#define HARD_FAULT_MASK   (ANOMALY_SIGNAL_LOSS   | \
                           ANOMALY_OVER_VOLTAGE  | \
                           ANOMALY_UNDER_VOLTAGE | \
                           ANOMALY_STUCK)
/* =========================================================================
 * Module state.
 * ========================================================================= */
static QueueHandle_t g_anom_q = NULL;
static StaticQueue_t g_anom_q_buf;
static uint8_t       g_anom_q_storage[sizeof(pzem_anomaly_t)];

/* Welford running mean/variance over voltage. */
static uint32_t g_n    = 0U;
static float    g_mean = 0.0f;
static float    g_m2   = 0.0f;   /* sum of squared deviations */

/* Stuck-detector state. */
static float    g_last_v      = 0.0f;
static uint32_t g_stuck_count = 0U;
static bool     g_have_last_v = false;

/* =========================================================================
 * Math helpers.
 * ========================================================================= */
static float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/* Welford (1962) incremental mean / variance:
 *   mean_n = mean_{n-1} + (x - mean_{n-1}) / n
 *   M2_n   = M2_{n-1} + (x - mean_{n-1}) * (x - mean_n)
 *   var    = M2_n / (n - 1)
 */
static void welford_update(float x)
{
    g_n += 1U;
    float delta  = x - g_mean;
    g_mean      += delta / (float) g_n;
    float delta2 = x - g_mean;
    g_m2        += delta * delta2;
}

static float welford_variance(void)
{
    if (g_n < 2U) return 0.0f;
    return g_m2 / (float)(g_n - 1U);
}
/* =========================================================================
 * Scoring stage. Seam where a TinyML model will plug in.
 *
 * Today:  monotonic mapping of |z_V| into [0, 1].
 * Future: return tflite_invoke(features);
 * ========================================================================= */
static float anomaly_score(const float * features)
{
    float abs_z = fabsf(features[5]);   /* index 5 = |z_V| */
    return clampf(abs_z / ZSCORE_SCORE_DIVISOR, 0.0f, 1.0f);
}

/* Map (flags, score) to a 3-level severity for the UI. */
static anomaly_state_t classify_state(uint32_t flags, float score)
{
    if (0U != (flags & HARD_FAULT_MASK)) return ANOMALY_STATE_FAULT;
    if (score >= SCORE_FAULT)            return ANOMALY_STATE_FAULT;
    if ((ANOMALY_NONE != flags) || (score >= SCORE_WARNING))
    {
        return ANOMALY_STATE_WARNING;
    }
    return ANOMALY_STATE_NORMAL;
}

const char * ANOMALY_StateLabel(anomaly_state_t s)
{
    switch (s)
    {
        case ANOMALY_STATE_WARNING: return "WARNING";
        case ANOMALY_STATE_FAULT:   return "FAULT";
        case ANOMALY_STATE_NORMAL:
        default:                    return "NORMAL";
    }
}
/* =========================================================================
 * Public API.
 * ========================================================================= */
void ANOMALY_QueueInit(void)
{
    if (NULL == g_anom_q)
    {
        g_anom_q = xQueueCreateStatic(1U,
                                      sizeof(pzem_anomaly_t),
                                      g_anom_q_storage,
                                      &g_anom_q_buf);
    }
}

bool ANOMALY_GetLatest(pzem_anomaly_t * out)
{
    if ((NULL == out) || (NULL == g_anom_q))
    {
        return false;
    }
    return pdTRUE == xQueuePeek(g_anom_q, out, 0);
}

/* =========================================================================
 * Task entry.
 * ========================================================================= */
void ANOMALY_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    ANOMALY_QueueInit();
    pzem_data_t    sample;
    pzem_anomaly_t out;
    TickType_t     last_wake = xTaskGetTickCount();
    while (1)
    {
        memset(&out, 0, sizeof(out));
        out.tick_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        /* 1) Signal-loss check. */
        uint32_t last_pzem = PZEM_GetLastSampleMs();
        bool     have_data = PZEM_GetData(&sample);
        if ((!have_data) ||
            (0U == last_pzem) ||
            ((out.tick_ms - last_pzem) > ANOMALY_SIGNAL_TIMEOUT_MS))
        {
            out.flags |= ANOMALY_SIGNAL_LOSS;
            out.score  = 1.0f;
            out.state = ANOMALY_STATE_FAULT;
            (void) xQueueOverwrite(g_anom_q, &out);
            vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(ANOMALY_PERIOD_MS));
            continue;
        }
        /* 2) Build the feature vector. */
        out.features[0] = sample.voltage_v;
        out.features[1] = sample.current_a;
        out.features[2] = sample.power_w;
        out.features[3] = sample.frequency_hz;
        out.features[4] = sample.power_factor;

        /* 3) Running stats on voltage and z-score. */
        welford_update(sample.voltage_v);
        float var = welford_variance();
        if (var < VAR_FLOOR) var = VAR_FLOOR;
        float std_dev   = sqrtf(var);
        float z         = (sample.voltage_v - g_mean) / std_dev;
        out.features[5] = fabsf(z);
        /* 4) Rule-based flags. */
        if (sample.voltage_v > V_MAX_VOLTS) out.flags |= ANOMALY_OVER_VOLTAGE;
        if (sample.voltage_v < V_MIN_VOLTS) out.flags |= ANOMALY_UNDER_VOLTAGE;
        if ((sample.frequency_hz < F_MIN_HZ) ||
            (sample.frequency_hz > F_MAX_HZ))
        {
            out.flags |= ANOMALY_FREQ_OUT;
        }

        if ((sample.power_w >= PF_LOAD_THRESHOLD_W) &&
            (sample.power_factor < PF_LOW_LIMIT))
        {
            out.flags |= ANOMALY_PF_LOW;
        }
        if ((g_n >= WELFORD_WARMUP_SAMPLES) && (fabsf(z) > ZSCORE_TRIGGER))
        {
            out.flags |= ANOMALY_SUDDEN_DELTA;
        }
        /* 5) Stuck detector. */
        if (g_have_last_v)
        {
            if (fabsf(sample.voltage_v - g_last_v) < STUCK_EPSILON_V)
            {
                g_stuck_count += 1U;
            }
            else
            {
                g_stuck_count = 0U;
            }
        }
        g_last_v      = sample.voltage_v;
        g_have_last_v = true;
        if (g_stuck_count >= STUCK_WINDOW)
        {
            //out.flags |= ANOMALY_STUCK;
        }
        /* 6) Final score (placeholder for TinyML). */
        out.score = anomaly_score(out.features);
        if ((ANOMALY_NONE != out.flags) && (out.score < 0.5f))
        {
            out.score = 0.5f;   /* any hard rule trips at least moderate */
        }
        out.state = classify_state(out.flags,out.score);
        (void) xQueueOverwrite(g_anom_q, &out);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(ANOMALY_PERIOD_MS));
    }
}
