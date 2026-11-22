#pragma once

#include "system_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_now.h"
#include "esp_wifi.h"

// ─── ESP-NOW Config ────────────────────────────────────
#define ESPNOW_TX_INTERVAL_MS   100
#define ESPNOW_WIFI_CHANNEL     1
#define ESPNOW_MAX_RETRY        3

// ─── HMI Board MAC Address ─────────────────────────────
#define HMI_MAC_ADDR    {0x30, 0xed, 0xa0, 0xa0, 0x6f, 0x34}

// ─── Task Config ───────────────────────────────────────
#define ESPNOW_TASK_STACK       4096
#define ESPNOW_TASK_PRIORITY    5
#define ESPNOW_TASK_CORE        0

// ─── API ───────────────────────────────────────────────
void espnow_tx_init(void);
void espnow_tx_task(void *pvParameters);
void espnow_tx_set_peer_mac(uint8_t *mac);