#pragma once

#include "system_state.h"
#include "freertos/FreeRTOS.h"
#include "adc_sampling.h" 
#include "freertos/task.h"

// ─── Feature Extraction Config ─────────────────────────
#define FEATURE_WINDOW_SIZE     400    // samples per window
                                        // 2kHz x 20 cycles of 50Hz
#define FEATURE_WINDOW_OVERLAP  200     // 50% overlap
#define MIN_SAMPLES_REQUIRED    400     // wait until full window

// ─── Task Config ───────────────────────────────────────
#define FEATURE_TASK_STACK      6144
#define FEATURE_TASK_PRIORITY   8
#define FEATURE_TASK_CORE       1

// ─── API ───────────────────────────────────────────────
void feature_extraction_init(void);
void feature_extraction_task(void *pvParameters);

// ─── Internal helpers (exposed for testing) ────────────
float compute_rms(float *samples, uint32_t count);
float compute_peak(float *samples, uint32_t count);
float compute_crest_factor(float rms, float peak);
float compute_rate_of_change(float *samples, uint32_t count);
float compute_zero_crossing_rate(float *samples, uint32_t count);