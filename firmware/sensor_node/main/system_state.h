#pragma once

#include "shared_types.h"
#include "ring_buffer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

// ─── Queue Depths ──────────────────────────────────────
#define FEATURE_QUEUE_DEPTH   8
#define DETECTION_QUEUE_DEPTH 8

// ─── System State Struct ───────────────────────────────
typedef struct {

    // ── Live Readings ──────────────────────────────────
    float rms_current;
    float peak_current;
    float crest_factor;
    float rate_of_change;
    float zscore;
    float anomaly_score;

    // ── Fault Info ─────────────────────────────────────
    system_state_e  state;
    fault_type_t    fault_type;
    bool            layer1_fired;
    bool            layer2_fired;
    bool            layer3_fired;
    uint32_t        fault_timestamp_ms;
    uint32_t        fault_count;

    // ── Config ─────────────────────────────────────────
    pump_config_t config;

    // ── Demo Mode ──────────────────────────────────────
    bool         demo_mode;
    fault_type_t demo_fault;

    // ── Statistics ─────────────────────────────────────
    float    rolling_mean;          // for Z-Score
    float    rolling_variance;      // for Z-Score
    uint32_t sample_count;          // Welford counter

    // ── Uptime ─────────────────────────────────────────
    uint32_t uptime_ms;

    // ── MQTT flag ──────────────────────────────────────
    bool mqtt_enabled;

    // ── Shared Resources ───────────────────────────────
    SemaphoreHandle_t mutex;
    QueueHandle_t     feature_queue;    // Task2 → Task3
    QueueHandle_t     detection_queue;  // Task3 → Task4

    // ── Ring Buffer ────────────────────────────────────
    ring_buffer_t adc_ring_buffer;

} system_state_t;

// ─── Global instance ───────────────────────────────────
extern system_state_t g_state;

// ─── API ───────────────────────────────────────────────
void system_state_init(void);
void system_state_lock(void);
void system_state_unlock(void);
void system_state_reset_fault(void);
pump_config_t system_state_get_default_config(void);