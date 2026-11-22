#include "espnow_tx.h"
#include "http_alert.h"
#include "detection_engine.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "ESPNOW_TX";

static uint8_t s_peer_mac[6] = HMI_MAC_ADDR;
static bool    s_peer_added  = false;

// ─── Send callback ─────────────────────────────────────
static void espnow_send_cb(const uint8_t *mac_addr,
                            esp_now_send_status_t status)
{
    if (status != ESP_NOW_SEND_SUCCESS) {
        ESP_LOGW(TAG, "Send failed to "
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2],
                 mac_addr[3], mac_addr[4], mac_addr[5]);
    }
}

// ─── Receive callback (config packets from HMI) ────────
static void espnow_recv_cb(
    const esp_now_recv_info_t *recv_info,
    const uint8_t *data, int len)
{
    if (len != sizeof(config_packet_t)) {
        ESP_LOGW(TAG, "Unexpected packet size %d", len);
        return;
    }

    config_packet_t cfg;
    memcpy(&cfg, data, sizeof(config_packet_t));

    system_state_lock();
    g_state.config.zscore_threshold   = cfg.zscore_threshold;
    g_state.config.anomaly_threshold  = cfg.anomaly_threshold;
    g_state.config.debounce_ms        = cfg.debounce_ms;
    g_state.config.rated_current_amps = cfg.rated_current_amps;
    g_state.demo_mode                 = cfg.demo_mode;
    g_state.demo_fault = (fault_type_t)cfg.demo_fault;
    system_state_unlock();

    if (cfg.recalibrate) {
        detection_engine_start_calibration();
        ESP_LOGI(TAG, "Recalibration triggered from HMI");
    }

    ESP_LOGI(TAG, "Config updated — Z=%.2f IF=%.2f"
                  " deb=%lu",
             cfg.zscore_threshold,
             cfg.anomaly_threshold,
             cfg.debounce_ms);
}

// ─── WiFi init ─────────────────────────────────────────
static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(
                        ESPNOW_WIFI_CHANNEL,
                        WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "WiFi init done — channel %d",
             ESPNOW_WIFI_CHANNEL);
}

// ─── Add peer ──────────────────────────────────────────
static void add_peer(void)
{
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, s_peer_mac, 6);
    peer.channel = ESPNOW_WIFI_CHANNEL;
    peer.ifidx   = ESP_IF_WIFI_STA;
    peer.encrypt = false;

    esp_err_t err = esp_now_add_peer(&peer);
    if (err == ESP_OK) {
        s_peer_added = true;
        ESP_LOGI(TAG, "Peer added: "
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 s_peer_mac[0], s_peer_mac[1],
                 s_peer_mac[2], s_peer_mac[3],
                 s_peer_mac[4], s_peer_mac[5]);
    } else {
        ESP_LOGE(TAG, "Failed to add peer: %s",
                 esp_err_to_name(err));
    }
}

// ─── Build telemetry packet ────────────────────────────
static void build_packet(espnow_packet_t *pkt)
{
    system_state_lock();
    pkt->rms_current     = g_state.rms_current;
    pkt->crest_factor    = g_state.crest_factor;
    pkt->zscore          = g_state.zscore;
    pkt->anomaly_score   = g_state.anomaly_score;
    pkt->rate_of_change  = g_state.rate_of_change;
    pkt->system_state    = (uint8_t)g_state.state;
    pkt->fault_type      = (uint8_t)g_state.fault_type;
    pkt->layer1_fired    = g_state.layer1_fired;
    pkt->layer2_fired    = g_state.layer2_fired;
    pkt->layer3_fired    = g_state.layer3_fired;
    pkt->fault_timestamp = g_state.fault_timestamp_ms;
    pkt->rated_current   = g_state.config.rated_current_amps;
    pkt->uptime_ms       = esp_timer_get_time() / 1000;
    system_state_unlock();
}

// ─── Send job confirmation to HMI ─────────────────────
// Called from alert_manager after Simpro job created
void espnow_tx_send_job_confirm(job_result_t *result,
                                 float rms,
                                 uint8_t fault)
{
    job_confirm_packet_t pkt = {0};
    pkt.packet_type  = 0xCA;   // CAFM marker
    pkt.success      = result->success;
    pkt.job_id       = result->job_id;
    pkt.fault_type   = fault;
    pkt.rms_at_fault = rms;

    strncpy(pkt.job_number, result->job_number,
            sizeof(pkt.job_number) - 1);
    strncpy(pkt.created_at, result->created_at,
            sizeof(pkt.created_at) - 1);

    esp_err_t err = esp_now_send(
        s_peer_mac,
        (uint8_t *)&pkt,
        sizeof(job_confirm_packet_t));

    if (err == ESP_OK) {
        ESP_LOGI(TAG,
                 "Job confirm sent to HMI — #%d",
                 result->job_id);
    } else {
        ESP_LOGE(TAG,
                 "Job confirm send failed: %s",
                 esp_err_to_name(err));
    }
}

// ─── Set peer MAC ──────────────────────────────────────
void espnow_tx_set_peer_mac(uint8_t *mac)
{
    memcpy(s_peer_mac, mac, 6);
    if (s_peer_added) {
        esp_now_del_peer(s_peer_mac);
        s_peer_added = false;
    }
    add_peer();
}

// ─── Init ──────────────────────────────────────────────
void espnow_tx_init(void)
{
    wifi_init();

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(
                        espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(
                        espnow_recv_cb));

    add_peer();

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    ESP_LOGI(TAG, "Sensor node MAC: "
             "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "ESP-NOW TX init done");
}

// ─── TX Task ───────────────────────────────────────────
void espnow_tx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "ESP-NOW TX task started on core %d",
             xPortGetCoreID());

    espnow_packet_t pkt;

    while (1) {
        build_packet(&pkt);

        esp_err_t err = esp_now_send(
            s_peer_mac,
            (uint8_t *)&pkt,
            sizeof(espnow_packet_t));

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "esp_now_send failed: %s",
                     esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(ESPNOW_TX_INTERVAL_MS));
    }
}