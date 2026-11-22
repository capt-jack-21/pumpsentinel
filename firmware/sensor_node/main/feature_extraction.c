#include "feature_extraction.h"
#include "adc_sampling.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>
#include <string.h>

static const char *TAG = "FEATURE";

static float s_window[FEATURE_WINDOW_SIZE];

// ─── Compute RMS current ───────────────────────────────
// Ring buffer stores raw voltage (unipolar 0 to ~0.68V)
// Use peak-based formula matching adc_sampling_task exactly
float compute_rms(float *samples, uint32_t count)
{
    float peak_v = 0.0f;
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i] > peak_v) peak_v = samples[i];
    }
    float voltage_rms = (peak_v * 0.707f) / 2.0f;
    return voltage_rms * SENSOR_RANGE_A * SENSOR_CALIBRATION;
}

// ─── Compute Peak current ──────────────────────────────
// Peak = RMS * sqrt(2) — keeps CF meaningful
float compute_peak(float *samples, uint32_t count)
{
    return compute_rms(samples, count) * 1.414f;
}

// ─── Compute Crest Factor ──────────────────────────────
float compute_crest_factor(float rms, float peak)
{
    if (rms < 0.01f) return 0.0f;
    return peak / rms;
}

// ─── Compute Rate of Change ────────────────────────────
float compute_rate_of_change(float *samples, uint32_t count)
{
    if (count < 2) return 0.0f;
    float sum_delta = 0.0f;
    for (uint32_t i = 1; i < count; i++) {
        sum_delta += fabsf(samples[i] - samples[i - 1]);
    }
    float avg_delta = sum_delta / (count - 1);
    return avg_delta * SAMPLE_RATE_HZ;
}

// ─── Compute Zero Crossing Rate ────────────────────────
float compute_zero_crossing_rate(float *samples, uint32_t count)
{
    if (count < 2) return 0.0f;
    uint32_t crossings = 0;
    for (uint32_t i = 1; i < count; i++) {
        if ((samples[i - 1] >= 0.0f && samples[i] < 0.0f) ||
            (samples[i - 1] <  0.0f && samples[i] >= 0.0f)) {
            crossings++;
        }
    }
    float window_duration_s = (float)count / SAMPLE_RATE_HZ;
    return (float)crossings / window_duration_s;
}

// ─── Welford update ────────────────────────────────────
static void welford_update(float new_value)
{
    system_state_lock();
    g_state.sample_count++;
    float delta  = new_value - g_state.rolling_mean;
    g_state.rolling_mean     += delta / g_state.sample_count;
    float delta2 = new_value - g_state.rolling_mean;
    g_state.rolling_variance += delta * delta2;
    system_state_unlock();
}

// ─── Init ──────────────────────────────────────────────
void feature_extraction_init(void)
{
    memset(s_window, 0, sizeof(s_window));
    ESP_LOGI(TAG, "Feature extraction init done");
    ESP_LOGI(TAG, "Window: %d samples, overlap: %d",
             FEATURE_WINDOW_SIZE, FEATURE_WINDOW_OVERLAP);
}

// ─── Feature Extraction Task ───────────────────────────
void feature_extraction_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Feature task started on core %d",
             xPortGetCoreID());

    feature_vector_t fv;

    while (1) {
        // ── Wait for full window ───────────────────────
        if (ring_buffer_count(&g_state.adc_ring_buffer)
                < FEATURE_WINDOW_SIZE) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // ── Read window ────────────────────────────────
        bool ok = ring_buffer_read_batch(
                      &g_state.adc_ring_buffer,
                      s_window,
                      FEATURE_WINDOW_SIZE);
        if (!ok) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // ── Compute features ───────────────────────────
        fv.rms_current     = compute_rms(s_window, FEATURE_WINDOW_SIZE);
        fv.peak_current    = compute_peak(s_window, FEATURE_WINDOW_SIZE);
        fv.crest_factor    = compute_crest_factor(
                                 fv.rms_current, fv.peak_current);
        fv.rate_of_change  = compute_rate_of_change(
                                 s_window, FEATURE_WINDOW_SIZE);
        fv.zero_cross_rate = compute_zero_crossing_rate(
                                 s_window, FEATURE_WINDOW_SIZE);
        fv.timestamp_ms    = esp_timer_get_time() / 1000;

        // ── Update rolling stats ───────────────────────
        welford_update(fv.rms_current);

        // ── Update global state ────────────────────────
        system_state_lock();
        g_state.rms_current    = fv.rms_current;
        g_state.peak_current   = fv.peak_current;
        g_state.crest_factor   = fv.crest_factor;
        g_state.rate_of_change = fv.rate_of_change;
        system_state_unlock();

        ESP_LOGI(TAG, "FEAT_CSV,%lu,%.3f,%.3f,%.3f,%.1f",
                 fv.timestamp_ms,
                 fv.rms_current,
                 fv.peak_current,
                 fv.crest_factor,
                 fv.rate_of_change);

        // ── Push to detection queue ────────────────────
        xQueueSend(g_state.feature_queue, &fv, pdMS_TO_TICKS(10));

        // ── Overlap delay ──────────────────────────────
        vTaskDelay(pdMS_TO_TICKS(
            (FEATURE_WINDOW_OVERLAP * 1000) / SAMPLE_RATE_HZ));
    }
}