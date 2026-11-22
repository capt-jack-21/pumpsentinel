#include "system_state.h"
#include <string.h>

// ─── Global instance ───────────────────────────────────
system_state_t g_state;

// ─── Default config values ─────────────────────────────
pump_config_t system_state_get_default_config(void)
{
    pump_config_t cfg = {
        .zscore_threshold    = 5.0f,
        .anomaly_threshold   = 0.65f,
        .debounce_ms         = 3000,
        .rated_current_amps  = 3.2f,
        .motor_hp            = 1,       // 1HP default
        .pump_type           = 0,       // Submersible
        .warning_pct         = 1.3f,    // 30% over rated
        .fault_pct           = 1.8f,    // 80% over rated
        .auto_reset          = true,
        .brightness          = 80,
    };
    return cfg;
}

// ─── Init ──────────────────────────────────────────────
void system_state_init(void)
{
    memset(&g_state, 0, sizeof(system_state_t));

    // Live readings
    g_state.rms_current    = 0.0f;
    g_state.peak_current   = 0.0f;
    g_state.crest_factor   = 0.0f;
    g_state.rate_of_change = 0.0f;
    g_state.zscore         = 0.0f;
    g_state.anomaly_score  = 0.0f;

    // Fault info
    g_state.state               = STATE_NORMAL;
    g_state.fault_type          = FAULT_NONE;
    g_state.layer1_fired        = false;
    g_state.layer2_fired        = false;
    g_state.layer3_fired        = false;
    g_state.fault_timestamp_ms  = 0;
    g_state.fault_count         = 0;

    // Config
    g_state.config = system_state_get_default_config();

    // Demo mode
    g_state.demo_mode  = false;
    g_state.demo_fault = FAULT_NONE;

    // Statistics
    g_state.rolling_mean     = 0.0f;
    g_state.rolling_variance = 0.0f;
    g_state.sample_count     = 0;

    // Uptime
    g_state.uptime_ms = 0;

    // MQTT
    g_state.mqtt_enabled = false;

    // Mutex
    g_state.mutex = xSemaphoreCreateMutex();
    configASSERT(g_state.mutex != NULL);

    // Queues
    g_state.feature_queue   = xQueueCreate(FEATURE_QUEUE_DEPTH,
                                            sizeof(feature_vector_t));
    g_state.detection_queue = xQueueCreate(DETECTION_QUEUE_DEPTH,
                                            sizeof(detection_result_t));
    configASSERT(g_state.feature_queue   != NULL);
    configASSERT(g_state.detection_queue != NULL);

    // Ring buffer
    ring_buffer_init(&g_state.adc_ring_buffer);
}

// ─── Lock / Unlock ─────────────────────────────────────
void system_state_lock(void)
{
    xSemaphoreTake(g_state.mutex, portMAX_DELAY);
}

void system_state_unlock(void)
{
    xSemaphoreGive(g_state.mutex);
}

// ─── Reset fault state ─────────────────────────────────
void system_state_reset_fault(void)
{
    system_state_lock();
    g_state.state        = STATE_NORMAL;
    g_state.fault_type   = FAULT_NONE;
    g_state.layer1_fired = false;
    g_state.layer2_fired = false;
    g_state.layer3_fired = false;
    system_state_unlock();
}