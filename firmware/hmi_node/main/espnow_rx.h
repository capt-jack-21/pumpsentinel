#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ─── Sensor node MAC ───────────────────────────────────
#define SENSOR_MAC_ADDR     {0x7c, 0xdf, 0xa1, 0xe2, 0xf2, 0x30}
#define ESPNOW_WIFI_CHANNEL 1

// ─── Task config ───────────────────────────────────────
#define ESPNOW_RX_TASK_STACK    4096
#define ESPNOW_RX_TASK_PRIORITY 5
#define ESPNOW_RX_TASK_CORE     1

// ─── Timeout ───────────────────────────────────────────
#define SENSOR_TIMEOUT_MS   2000    // 2s without packet = disconnected

// ─── API ───────────────────────────────────────────────
void espnow_rx_init(void);
void espnow_rx_task(void *pvParameters);
