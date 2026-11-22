#include "adc_sampling.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>

static const char *TAG = "ADC";

static adc_oneshot_unit_handle_t s_adc_handle;
static adc_cali_handle_t         s_cali_handle;
static bool                      s_cali_enabled = false;

// ─── Calibration init ──────────────────────────────────
static void adc_calibration_init(void)
{
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT,
        .chan     = ADC_CHANNEL,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };

    esp_err_t ret = adc_cali_create_scheme_curve_fitting(
                        &cali_cfg, &s_cali_handle);

    if (ret == ESP_OK) {
        s_cali_enabled = true;
        ESP_LOGI(TAG, "ADC calibration enabled");
    } else {
        s_cali_enabled = false;
        ESP_LOGW(TAG, "ADC calibration not available");
    }
}

// ─── Init ──────────────────────────────────────────────
void adc_sampling_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg,
                                         &s_adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
                        s_adc_handle, ADC_CHANNEL, &chan_cfg));

    adc_calibration_init();

    ESP_LOGI(TAG, "ADC init done — GPIO7, 1kHz, 20A range");

    // ── Print CSV header ───────────────────────────────
    ESP_LOGI(TAG, "CSV_HEADER,timestamp_ms,peak_v,"
                  "vrms_v,current_a,raw_adc,mv");
}

// ─── Raw ADC to voltage ────────────────────────────────
static float adc_raw_to_voltage(int raw)
{
    if (s_cali_enabled) {
        int voltage_mv = 0;
        adc_cali_raw_to_voltage(s_cali_handle,
                                 raw, &voltage_mv);
        return voltage_mv / 1000.0f;
    }
    return (raw / ADC_MAX_COUNT) * SENSOR_VREF;
}

// ─── Peak voltage to RMS current ───────────────────────
float adc_raw_to_current(int raw)
{
    float voltage     = adc_raw_to_voltage(raw);
    float voltage_rms = (voltage * 0.707f) / 2.0f;
    return voltage_rms * SENSOR_RANGE_A;
}

// ─── ADC Sampling Task ─────────────────────────────────
void adc_sampling_task(void *pvParameters)
{
    ESP_LOGI(TAG, "ADC task started on core %d",
             xPortGetCoreID());

    int   raw      = 0;
    int   peak_raw = 0;
    float voltage  = 0.0f;
    float peak     = 0.0f;

    // Log counter — log every N windows
    uint32_t log_counter = 0;

    while (1) {
        peak     = 0.0f;
        peak_raw = 0;

        // ── Sample 400 points — find peak ──────────────
        for (int i = 0; i < SAMPLES_PER_MEASUREMENT; i++) {
            adc_oneshot_read(s_adc_handle,
                             ADC_CHANNEL, &raw);
            voltage = adc_raw_to_voltage(raw);

            if (voltage > peak) {
                peak     = voltage;
                peak_raw = raw;
            }

            ring_buffer_write(&g_state.adc_ring_buffer,
                              voltage);

            vTaskDelay(pdMS_TO_TICKS(1));
        }

        // ── Convert peak to RMS current ────────────────
        float voltage_rms = (peak * 0.707f) / 2.0f;
        float current     = voltage_rms * SENSOR_RANGE_A
                            * SENSOR_CALIBRATION;

        // ── Update global state ────────────────────────
        uint32_t now_ms = esp_timer_get_time() / 1000;

        system_state_lock();
        g_state.uptime_ms = now_ms;
        system_state_unlock();

        // ── CSV log every window ───────────────────────
        // Format: CSV,timestamp_ms,peak_v,vrms_v,
        //         current_a,peak_raw,peak_mv
        int peak_mv = 0;
        if (s_cali_enabled) {
            adc_cali_raw_to_voltage(s_cali_handle,
                                     peak_raw, &peak_mv);
        }

        ESP_LOGI(TAG, "CSV,%lu,%.4f,%.4f,%.4f,%d,%d",
                 now_ms,
                 peak,
                 voltage_rms,
                 current,
                 peak_raw,
                 peak_mv);

        // ── Human readable log every 10 windows ────────
        log_counter++;
        if (log_counter % 10 == 0) {
            ESP_LOGI(TAG,
                     "Peak=%.3fV  Vrms=%.3fV  I=%.3fA",
                     peak, voltage_rms, current);
        }
    }
}