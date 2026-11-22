# PumpSentinel – architecture

## Sensor node (ESP32-S3)

FreeRTOS tasks, pinned across both cores:

| Task | Core | Role |
|------|------|------|
| `adc_task` | 1 | Samples the current sensor at 1 kHz into a ring buffer (400-sample windows) |
| `feature_task` | 1 | RMS, peak, crest factor, rate of change, zero-crossing rate |
| `detection_task` | 0 | Boot calibration, then Z-score → Isolation Forest → signature match |
| `alert_task` | 0 | Debounce, state machine, stall webhook (30-minute cooldown) |
| `espnow_task` | 0 | 10 Hz telemetry to the HMI; receives config packets |

Shared state lives in `system_state.[ch]` behind a mutex.

## HMI node (Waveshare ESP32-S3 4.3")

LVGL v8 tab view: **Dashboard** (live current, status, layer indicators), **Fault** (fault detail),
**Config** (thresholds, system info), **PumpIQ** (AI health / anomaly view), **CAFM** (work-order status).
UI state is refreshed every 200 ms from `g_hmi`, which `espnow_rx` fills.

## ESP-NOW protocol (`shared_types.h`)

| Packet | Direction | Purpose |
|--------|-----------|---------|
| `espnow_packet_t` | Sensor → HMI | Telemetry: RMS, crest factor, z-score, anomaly score, state, fault, layer flags |
| `job_confirm_packet_t` (`0xCA`) | Sensor → HMI | Result of the webhook call: success, job id/number, timestamp, fault, RMS |
| `config_packet_t` | HMI → Sensor | Thresholds, debounce, rated current, demo mode, recalibrate |

## Stall → work-order flow

Rotor stall confirmed → sensor node joins WiFi → HTTP POST to the configured webhook (n8n workflow) →
CAFM job created → result sent back to the HMI over ESP-NOW → CAFM tab updates.
