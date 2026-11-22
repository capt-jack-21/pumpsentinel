#include "alert_manager.h"
#include "http_alert.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "ALERT";

static fault_event_t s_fault_history[ALERT_HISTORY_SIZE];
static uint32_t      s_fault_count    = 0;
static uint32_t      s_fault_start_ms = 0;
static bool          s_fault_pending  = false;
static fault_type_t  s_pending_fault  = FAULT_NONE;

// ─── Extern — send job confirmation via ESP-NOW ────────
extern void espnow_tx_send_job_confirm(job_result_t *result,
                                        float rms,
                                        uint8_t fault);

// ─── Resolve fault type ────────────────────────────────
static fault_type_t resolve_fault_type(detection_result_t *dr)
{
    if (dr->layer3_fired) {
        return (fault_type_t)dr->signature_match;
    }
    return FAULT_NONE;
}

// ─── Resolve severity ──────────────────────────────────
static system_state_e resolve_severity(detection_result_t *dr)
{
    if (dr->layer3_fired)                        return STATE_FAULT;
    if (dr->layer1_fired || dr->layer2_fired)    return STATE_WARNING;
    return STATE_NORMAL;
}

// ─── Record fault ──────────────────────────────────────
static void record_fault(detection_result_t *dr,
                          fault_type_t fault_type,
                          system_state_e severity)
{
    uint32_t idx = s_fault_count % ALERT_HISTORY_SIZE;
    s_fault_history[idx].fault_type       = fault_type;
    s_fault_history[idx].severity         = severity;
    s_fault_history[idx].rms_at_fault     = g_state.rms_current;
    s_fault_history[idx].zscore_at_fault  = dr->zscore;
    s_fault_history[idx].anomaly_at_fault = dr->anomaly_score;
    s_fault_history[idx].layer1_fired     = dr->layer1_fired;
    s_fault_history[idx].layer2_fired     = dr->layer2_fired;
    s_fault_history[idx].layer3_fired     = dr->layer3_fired;
    s_fault_history[idx].timestamp_ms     =
        esp_timer_get_time() / 1000;
    s_fault_count++;
    ESP_LOGW(TAG,
             "Fault recorded — type=%d severity=%d count=%lu",
             fault_type, severity, s_fault_count);
}

// ─── Raise HTTP alert for rotor stall ─────────────────
static void raise_stall_alert(float rms, uint32_t uptime)
{
    if (!http_alert_can_raise()) {
        uint32_t rem = http_alert_cooldown_remaining_ms();
        ESP_LOGI(TAG,
                 "Stall alert suppressed — "
                 "cooldown %lum remaining",
                 rem / 60000);
        return;
    }

    ESP_LOGI(TAG, "Raising stall alert to Simpro"
                  " — RMS=%.2fA", rms);

    job_result_t result =
        http_alert_raise_stall(rms, uptime);

    if (result.success) {
        ESP_LOGI(TAG, "Simpro job created — #%d %s",
                 result.job_id, result.job_number);
    } else {
        ESP_LOGE(TAG, "Simpro job failed — %s",
                 result.error);
    }

    // Send job confirmation back to HMI via ESP-NOW
    espnow_tx_send_job_confirm(&result, rms,
                                FAULT_STALL);
}

// ─── Init ──────────────────────────────────────────────
void alert_manager_init(void)
{
    memset(s_fault_history, 0, sizeof(s_fault_history));
    s_fault_count    = 0;
    s_fault_pending  = false;
    s_fault_start_ms = 0;
    ESP_LOGI(TAG, "Alert manager init done");
}

fault_event_t alert_manager_get_last_fault(void)
{
    if (s_fault_count == 0) {
        fault_event_t empty = {0};
        return empty;
    }
    uint32_t idx = (s_fault_count - 1) % ALERT_HISTORY_SIZE;
    return s_fault_history[idx];
}

uint32_t alert_manager_get_fault_count(void)
{
    return s_fault_count;
}

// ─── Alert Manager Task ────────────────────────────────
void alert_manager_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Alert task started on core %d",
             xPortGetCoreID());

    // Init HTTP alert — connects WiFi in background
    http_alert_init();

    detection_result_t dr;

    while (1) {
        if (xQueueReceive(g_state.detection_queue,
                          &dr,
                          pdMS_TO_TICKS(200)) != pdTRUE) {
            if (g_state.config.auto_reset &&
                g_state.state != STATE_NORMAL) {
                system_state_lock();
                g_state.state      = STATE_NORMAL;
                g_state.fault_type = FAULT_NONE;
                system_state_unlock();
                ESP_LOGI(TAG, "Auto reset — NORMAL");
            }
            continue;
        }

        fault_type_t   fault    = resolve_fault_type(&dr);
        system_state_e severity = resolve_severity(&dr);
        uint32_t       now_ms   =
            esp_timer_get_time() / 1000;

        if (severity == STATE_NORMAL) {
            s_fault_pending  = false;
            s_fault_start_ms = 0;

            system_state_lock();
            g_state.state        = STATE_NORMAL;
            g_state.fault_type   = FAULT_NONE;
            g_state.layer1_fired = false;
            g_state.layer2_fired = false;
            g_state.layer3_fired = false;
            system_state_unlock();

        } else if (severity == STATE_WARNING) {
            s_fault_pending  = false;
            s_fault_start_ms = 0;

            system_state_lock();
            g_state.state        = STATE_WARNING;
            g_state.fault_type   = FAULT_NONE;
            g_state.layer1_fired = dr.layer1_fired;
            g_state.layer2_fired = dr.layer2_fired;
            g_state.layer3_fired = false;
            system_state_unlock();

            ESP_LOGW(TAG, "WARNING — elevated current");

        } else {
            // ── Fault — L3 fired ──────────────────────
            if (!s_fault_pending) {
                s_fault_pending  = true;
                s_fault_start_ms = now_ms;
                s_pending_fault  = fault;

                system_state_lock();
                g_state.state        = STATE_FAULT;
                g_state.fault_type   = fault;
                g_state.layer1_fired = dr.layer1_fired;
                g_state.layer2_fired = dr.layer2_fired;
                g_state.layer3_fired = dr.layer3_fired;
                system_state_unlock();

                ESP_LOGW(TAG,
                         "FAULT type=%d debouncing...",
                         fault);

            } else if ((now_ms - s_fault_start_ms) >=
                        g_state.config.debounce_ms) {
                // ── Debounce passed — confirmed ────────
                system_state_lock();
                float rms    = g_state.rms_current;
                uint32_t up  = g_state.uptime_ms;
                system_state_unlock();

                system_state_lock();
                g_state.state              = STATE_FAULT;
                g_state.fault_type         = s_pending_fault;
                g_state.fault_timestamp_ms = now_ms;
                g_state.fault_count++;
                system_state_unlock();

                record_fault(&dr, s_pending_fault,
                             STATE_FAULT);

                ESP_LOGE(TAG, "FAULT CONFIRMED type=%d",
                         s_pending_fault);

                // ── Raise Simpro alert for STALL only ─
                if (s_pending_fault == FAULT_STALL) {
                    raise_stall_alert(rms, up);
                }

                s_fault_pending = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(ALERT_CHECK_INTERVAL_MS));
    }
}