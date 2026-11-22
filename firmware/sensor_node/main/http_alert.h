#pragma once

#include <stdbool.h>
#include <stdint.h>

// ─── Credentials & webhook ─────────────────────────────
// Set via `idf.py menuconfig` -> "PumpSentinel Sensor Node".
// Values are stored in the git-ignored sdkconfig, never in source.
#define SENSOR_WIFI_SSID        CONFIG_PUMPSENTINEL_WIFI_SSID
#define SENSOR_WIFI_PASSWORD    CONFIG_PUMPSENTINEL_WIFI_PASSWORD
#define STALL_WEBHOOK_URL       CONFIG_PUMPSENTINEL_STALL_WEBHOOK_URL

// ─── Cooldown — 30 minutes ─────────────────────────────
#define STALL_COOLDOWN_MS   (30 * 60 * 1000)

// ─── Job result ────────────────────────────────────────
typedef struct {
    bool    success;
    int     job_id;
    char    job_number[32];
    char    created_at[32];
    char    error[128];
} job_result_t;

// ─── API ───────────────────────────────────────────────
void         http_alert_init(void);
bool         http_alert_can_raise(void);
job_result_t http_alert_raise_stall(float    rms_current,
                                     uint32_t uptime_ms);
uint32_t     http_alert_cooldown_remaining_ms(void);