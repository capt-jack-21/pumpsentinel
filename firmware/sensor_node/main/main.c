#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "system_state.h"
#include "ring_buffer.h"
#include "adc_sampling.h"
#include "feature_extraction.h"
#include "detection_engine.h"
#include "alert_manager.h"
#include "espnow_tx.h"

static const char *TAG = "MAIN";

// ─── Task handles ──────────────────────────────────────
static TaskHandle_t s_adc_task_handle       = NULL;
static TaskHandle_t s_feature_task_handle   = NULL;
static TaskHandle_t s_detection_task_handle = NULL;
static TaskHandle_t s_alert_task_handle     = NULL;
static TaskHandle_t s_espnow_task_handle    = NULL;

// ─── NVS Init ──────────────────────────────────────────
static void nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition wiped and reinitialised");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS init done");
}

// ─── Spawn all tasks ───────────────────────────────────
static void spawn_tasks(void)
{
    BaseType_t ret;

    // Task 1 — ADC Sampling (Core 1, highest priority)
    ret = xTaskCreatePinnedToCore(
        adc_sampling_task,
        "adc_task",
        ADC_TASK_STACK,
        NULL,
        ADC_TASK_PRIORITY,
        &s_adc_task_handle,
        ADC_TASK_CORE);
    configASSERT(ret == pdPASS);
    ESP_LOGI(TAG, "ADC task spawned");

    // Task 2 — Feature Extraction (Core 1)
    ret = xTaskCreatePinnedToCore(
        feature_extraction_task,
        "feature_task",
        FEATURE_TASK_STACK,
        NULL,
        FEATURE_TASK_PRIORITY,
        &s_feature_task_handle,
        FEATURE_TASK_CORE);
    configASSERT(ret == pdPASS);
    ESP_LOGI(TAG, "Feature task spawned");

    // Task 3 — Detection Engine (Core 1)
    ret = xTaskCreatePinnedToCore(
        detection_engine_task,
        "detection_task",
        DETECTION_TASK_STACK,
        NULL,
        DETECTION_TASK_PRIORITY,
        &s_detection_task_handle,
        DETECTION_TASK_CORE);
    configASSERT(ret == pdPASS);
    ESP_LOGI(TAG, "Detection task spawned");

    // Task 4 — Alert Manager (Core 0)
    ret = xTaskCreatePinnedToCore(
        alert_manager_task,
        "alert_task",
        ALERT_TASK_STACK,
        NULL,
        ALERT_TASK_PRIORITY,
        &s_alert_task_handle,
        ALERT_TASK_CORE);
    configASSERT(ret == pdPASS);
    ESP_LOGI(TAG, "Alert task spawned");

    // Task 5 — ESP-NOW TX (Core 0)
    ret = xTaskCreatePinnedToCore(
        espnow_tx_task,
        "espnow_task",
        ESPNOW_TASK_STACK,
        NULL,
        ESPNOW_TASK_PRIORITY,
        &s_espnow_task_handle,
        ESPNOW_TASK_CORE);
    configASSERT(ret == pdPASS);
    ESP_LOGI(TAG, "ESP-NOW task spawned");
}

// ─── Health monitor loop ───────────────────────────────
static void health_monitor(void)
{
    while (1) {
        system_state_lock();
        float rms    = g_state.rms_current;
        float cf     = g_state.crest_factor;
        float zs     = g_state.zscore;
        float as_val = g_state.anomaly_score;
        uint8_t state  = g_state.state;
        uint8_t fault  = g_state.fault_type;
        uint32_t uptime = g_state.uptime_ms / 1000;
        system_state_unlock();

        ESP_LOGI(TAG,
            "── Health ──────────────────────────────────");
        ESP_LOGI(TAG,
            "  RMS: %.2fA  CF: %.2f  Z: %.2f  IF: %.2f",
            rms, cf, zs, as_val);
        ESP_LOGI(TAG,
            "  State: %d  Fault: %d  Uptime: %lus",
            state, fault, uptime);
        ESP_LOGI(TAG,
            "  Ring buffer: %lu samples",
            ring_buffer_count(&g_state.adc_ring_buffer));

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ─── app_main ──────────────────────────────────────────
void app_main(void)
{
        // Boot delay — gives monitor time to connect
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "╔══════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Pump Health Monitor v1.0       ║");
    ESP_LOGI(TAG, "║   Sensor Node — ESP32-S3         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════╝");

    // ── Step 1: NVS ────────────────────────────────────
    nvs_init();

    // ── Step 2: System state ───────────────────────────
    system_state_init();
    ESP_LOGI(TAG, "System state initialised");

    // ── Step 3: ADC ────────────────────────────────────
    adc_sampling_init();

    // ── Step 4: Feature extraction ─────────────────────
    feature_extraction_init();

    // ── Step 5: Detection engine ───────────────────────
    detection_engine_init();

    // ── Step 6: Alert manager ──────────────────────────
    alert_manager_init();

    // ── Step 7: ESP-NOW ────────────────────────────────
    espnow_tx_init();

    // ── Step 8: Spawn tasks ────────────────────────────
    ESP_LOGI(TAG, "Spawning tasks...");
    spawn_tasks();

    ESP_LOGI(TAG, "All tasks running — entering health monitor");

    // ── Step 9: Health monitor (runs in main task) ─────
    health_monitor();
}