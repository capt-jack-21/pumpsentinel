#pragma once

#include <stdint.h>
#include <stdbool.h>

// ─── Fault Types ───────────────────────────────────────
typedef enum {
    FAULT_NONE       = 0,
    FAULT_DRY_RUN    = 1,
    FAULT_MUD        = 2,
    FAULT_STALL      = 3,
    FAULT_CAVITATION = 4
} fault_type_t;

// ─── System State ──────────────────────────────────────
typedef enum {
    STATE_NORMAL  = 0,
    STATE_WARNING = 1,
    STATE_FAULT   = 2
} system_state_e;

// ─── Pump Config ───────────────────────────────────────
typedef struct {
    float    zscore_threshold;
    float    anomaly_threshold;
    uint32_t debounce_ms;
    float    rated_current_amps;
    uint8_t  motor_hp;
    uint8_t  pump_type;
    float    warning_pct;
    float    fault_pct;
    bool     auto_reset;
    uint8_t  brightness;
} pump_config_t;

// ─── Feature Vector ────────────────────────────────────
typedef struct {
    float    rms_current;
    float    peak_current;
    float    crest_factor;
    float    rate_of_change;
    float    zero_cross_rate;
    uint32_t timestamp_ms;
} feature_vector_t;

// ─── Detection Result ──────────────────────────────────
typedef struct {
    float   zscore;
    float   anomaly_score;
    uint8_t signature_match;
    bool    layer1_fired;
    bool    layer2_fired;
    bool    layer3_fired;
} detection_result_t;

// ─── ESP-NOW Telemetry Packet (Sensor → HMI) ──────────
typedef struct {
    float    rms_current;
    float    crest_factor;
    float    zscore;
    float    anomaly_score;
    float    rate_of_change;
    uint8_t  system_state;
    uint8_t  fault_type;
    bool     layer1_fired;
    bool     layer2_fired;
    bool     layer3_fired;
    uint32_t fault_timestamp;
    float    rated_current;
    uint32_t uptime_ms;
} __attribute__((packed)) espnow_packet_t;

// ─── ESP-NOW Job Confirmation Packet (Sensor → HMI) ───
// Sent after HTTP POST to n8n succeeds
// HMI CAFM tab populates from this
typedef struct {
    uint8_t  packet_type;           // always 0xCA (CAFM)
    bool     success;
    int32_t  job_id;
    char     job_number[32];
    char     created_at[24];
    uint8_t  fault_type;
    float    rms_at_fault;
} __attribute__((packed)) job_confirm_packet_t;

// ─── Config Packet (HMI → Sensor) ─────────────────────
typedef struct {
    float    zscore_threshold;
    float    anomaly_threshold;
    uint32_t debounce_ms;
    float    rated_current_amps;
    bool     demo_mode;
    uint8_t  demo_fault;
    bool     recalibrate;
} __attribute__((packed)) config_packet_t;