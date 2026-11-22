#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "shared_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ─── Live data from sensor node ────────────────────────
typedef struct {
    float           rms_current;
    float           crest_factor;
    float           zscore;
    float           anomaly_score;
    float           rate_of_change;
    uint8_t         system_state;
    uint8_t         fault_type;
    bool            layer1_fired;
    bool            layer2_fired;
    bool            layer3_fired;
    uint32_t        fault_timestamp;
    float           rated_current;
    uint32_t        uptime_ms;
    uint32_t        last_rx_ms;
    bool            connected;
    bool     stall_alert_pending;
    float    stall_rms_at_alert;
    uint32_t stall_uptime_at_alert;
    bool     job_received;          // true when job confirm arrived
bool     job_success;           // Simpro job created OK
int32_t  job_id;                // Simpro job ID
char     job_number[32];        // Simpro job reference
char     job_created_at[24];    // ISO timestamp
uint8_t  job_fault_type;        // fault that triggered job
float    job_rms;               // RMS at time of fault
} hmi_data_t;

// ─── Global state ──────────────────────────────────────
extern hmi_data_t        g_hmi;
extern SemaphoreHandle_t g_hmi_mutex;

static inline void hmi_lock(void)
{
    xSemaphoreTake(g_hmi_mutex, portMAX_DELAY);
}
static inline void hmi_unlock(void)
{
    xSemaphoreGive(g_hmi_mutex);
}