#pragma once

#include "system_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ─── Alert Manager Config ──────────────────────────────
#define ALERT_CHECK_INTERVAL_MS     100     // check every 100ms
#define ALERT_HISTORY_SIZE          10      // last 10 fault events

// ─── Fault History Entry ───────────────────────────────
typedef struct {
    fault_type_t    fault_type;
    system_state_e  severity;
    float           rms_at_fault;
    float           zscore_at_fault;
    float           anomaly_at_fault;
    bool            layer1_fired;
    bool            layer2_fired;
    bool            layer3_fired;
    uint32_t        timestamp_ms;
} fault_event_t;

// ─── Task Config ───────────────────────────────────────
#define ALERT_TASK_STACK        4096
#define ALERT_TASK_PRIORITY     6
#define ALERT_TASK_CORE         0

// ─── API ───────────────────────────────────────────────
void alert_manager_init(void);
void alert_manager_task(void *pvParameters);
fault_event_t alert_manager_get_last_fault(void);
uint32_t alert_manager_get_fault_count(void);