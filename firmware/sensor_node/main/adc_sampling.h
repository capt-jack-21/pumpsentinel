#pragma once

#include "system_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ─── ADC Config ────────────────────────────────────────
#define ADC_UNIT              ADC_UNIT_1
#define ADC_CHANNEL           ADC_CHANNEL_6   // GPIO7
#define ADC_ATTEN             ADC_ATTEN_DB_12
#define ADC_BITWIDTH          ADC_BITWIDTH_12

// ─── Sampling Config ───────────────────────────────────
#define SAMPLE_RATE_HZ            1000
#define SAMPLE_INTERVAL_US        (1000000 / SAMPLE_RATE_HZ)
#define SAMPLES_PER_MEASUREMENT   400

// ─── Sensor Config ─────────────────────────────────────
#define SENSOR_VREF               3.3f
#define SENSOR_RANGE_A            20.0f
#define SENSOR_CALIBRATION        1.5f    // calibration factor
#define ADC_MAX_COUNT             4095.0f

// ─── Task Config ───────────────────────────────────────
#define ADC_TASK_STACK        4096
#define ADC_TASK_PRIORITY     7
#define ADC_TASK_CORE         1

// ─── API ───────────────────────────────────────────────
void adc_sampling_init(void);
void adc_sampling_task(void *pvParameters);
float adc_raw_to_current(int raw);