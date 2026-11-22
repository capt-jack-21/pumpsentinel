#pragma once

#include "system_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ─── Isolation Forest Config ───────────────────────────
#define IF_NUM_TREES        10
#define IF_TREE_DEPTH       8
#define IF_SAMPLE_SIZE      32
#define IF_FEATURE_COUNT    3       // rms, crest_factor, rate_of_change

// ─── Signature Thresholds (RMS current, amps) ──────────
// Calibrated on a bench load bank with a single pump. Re-calibrate for your
// pump and current range before relying on these.
//
//   <= 1.5        Dry run
//   5.0 - 6.6     Normal
//   6.7 - 7.0     Warning
//   7.1 - 7.5     Mud ingestion
//   >= 7.6        Rotor stall
//
// Values between bands fall through to the Layer 1 / Layer 2 detectors.

// ─── Normal ────────────────────────────────────────────
#define SIG_NORMAL_RMS_MIN      5.0f
#define SIG_NORMAL_RMS_MAX      6.6f

// ─── Dry Run ───────────────────────────────────────────
// Orange banner — skip all detection layers
    #define SIG_DRY_RUN_RMS_MAX     1.5f

// ─── Warning ───────────────────────────────────────────
#define SIG_WARNING_RMS_MIN     6.7f
#define SIG_WARNING_RMS_MAX     7.0f

// ─── Mud Ingestion ─────────────────────────────────────
#define SIG_MUD_RMS_MIN         7.1f
#define SIG_MUD_RMS_MAX         7.5f

// ─── Rotor Stall ───────────────────────────────────────
#define SIG_STALL_RMS_MIN       7.6f

// ─── Baseline ──────────────────────────────────────────
#define BASELINE_RMS_MEAN       6.6f
#define BASELINE_RMS_STD        0.4f
#define BASELINE_CF_MEAN        1.41f
#define BASELINE_CF_STD         0.05f

// ─── Calibration Config ────────────────────────────────
#define CALIBRATION_DURATION_MS     30000
#define CALIBRATION_SAMPLE_COUNT    IF_SAMPLE_SIZE

// ─── Calibration State ─────────────────────────────────
typedef enum {
    CALIB_IDLE       = 0,
    CALIB_RUNNING    = 1,
    CALIB_COMPLETE   = 2,
    CALIB_FAILED     = 3
} calibration_state_t;

// ─── Task Config ───────────────────────────────────────
#define DETECTION_TASK_STACK      8192
#define DETECTION_TASK_PRIORITY   6
#define DETECTION_TASK_CORE       0

// ─── API ───────────────────────────────────────────────
void detection_engine_init(void);
void detection_engine_task(void *pvParameters);
void detection_engine_start_calibration(void);
calibration_state_t detection_engine_get_calib_state(void);
uint8_t detection_engine_get_calib_progress(void);

// ─── Internal (exposed for testing) ────────────────────
float   compute_zscore(float value, float mean, float variance);
float   compute_anomaly_score(float rms, float crest, float roc);
uint8_t match_fault_signature(float rms, float crest_factor);