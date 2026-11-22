#include "espnow_rx.h"
#include "hmi_state.h"
#include "shared_types.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "ESPNOW_RX";

// ─── Two queues — telemetry and job confirmations ──────
static QueueHandle_t s_rx_queue      = NULL;
static QueueHandle_t s_job_queue     = NULL;

// ─── Raw packet wrapper for routing ───────────────────
typedef struct {
    uint8_t  data[128];
    int      len;
} raw_packet_t;

static QueueHandle_t s_raw_queue = NULL;

// ─── Receive callback — routes by packet size ──────────
static void espnow_recv_cb(
    const esp_now_recv_info_t *recv_info,
    const uint8_t *data, int len)
{
    raw_packet_t raw = {0};
    if (len > (int)sizeof(raw.data)) return;
    memcpy(raw.data, data, len);
    raw.len = len;
    xQueueSendFromISR(s_raw_queue, &raw, NULL);
}

// ─── WiFi init — STA only, no AP connection ────────────
static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(
                        ESPNOW_WIFI_CHANNEL,
                        WIFI_SECOND_CHAN_NONE));

    ESP_LOGI(TAG, "WiFi STA started — channel %d",
             ESPNOW_WIFI_CHANNEL);
}

// ─── Init ──────────────────────────────────────────────
void espnow_rx_init(void)
{
    s_raw_queue = xQueueCreate(20, sizeof(raw_packet_t));
    s_rx_queue  = xQueueCreate(10,
                                sizeof(espnow_packet_t));
    s_job_queue = xQueueCreate(5,
                                sizeof(job_confirm_packet_t));

    wifi_init();

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(
                        espnow_recv_cb));

    esp_now_peer_info_t peer = {0};
    uint8_t sensor_mac[] = SENSOR_MAC_ADDR;
    memcpy(peer.peer_addr, sensor_mac, 6);
    peer.channel = ESPNOW_WIFI_CHANNEL;
    peer.ifidx   = ESP_IF_WIFI_STA;
    peer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    ESP_LOGI(TAG, "ESP-NOW RX init done");
    ESP_LOGI(TAG,
             "Listening for sensor: "
             "%02x:%02x:%02x:%02x:%02x:%02x",
             sensor_mac[0], sensor_mac[1],
             sensor_mac[2], sensor_mac[3],
             sensor_mac[4], sensor_mac[5]);
}

// ─── RX Task ───────────────────────────────────────────
void espnow_rx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "RX task started on core %d",
             xPortGetCoreID());

    raw_packet_t raw;

    while (1) {
        // ── Check for timeout ─────────────────────────
        uint32_t now = esp_timer_get_time() / 1000;

        hmi_lock();
        if (g_hmi.connected &&
            (now - g_hmi.last_rx_ms)
                > SENSOR_TIMEOUT_MS) {
            g_hmi.connected = false;
            ESP_LOGW(TAG, "Sensor node timeout");
        }
        hmi_unlock();

        // ── Wait for raw packet ───────────────────────
        if (xQueueReceive(s_raw_queue, &raw,
                          pdMS_TO_TICKS(100))
                != pdTRUE) {
            continue;
        }

        // ── Route by packet size ──────────────────────
        if (raw.len == sizeof(espnow_packet_t)) {
            // ── Telemetry packet ──────────────────────
            espnow_packet_t pkt;
            memcpy(&pkt, raw.data,
                   sizeof(espnow_packet_t));

            hmi_lock();
            g_hmi.rms_current     = pkt.rms_current;
            g_hmi.crest_factor    = pkt.crest_factor;
            g_hmi.zscore          = pkt.zscore;
            g_hmi.anomaly_score   = pkt.anomaly_score;
            g_hmi.rate_of_change  = pkt.rate_of_change;
            g_hmi.system_state    = pkt.system_state;
            g_hmi.fault_type      = pkt.fault_type;
            g_hmi.layer1_fired    = pkt.layer1_fired;
            g_hmi.layer2_fired    = pkt.layer2_fired;
            g_hmi.layer3_fired    = pkt.layer3_fired;
            g_hmi.fault_timestamp = pkt.fault_timestamp;
            g_hmi.rated_current   = pkt.rated_current;
            g_hmi.uptime_ms       = pkt.uptime_ms;
            g_hmi.last_rx_ms      = now;
            g_hmi.connected       = true;
            hmi_unlock();

            ESP_LOGD(TAG,
                     "RX telemetry: RMS=%.2fA"
                     " State=%d Fault=%d",
                     pkt.rms_current,
                     pkt.system_state,
                     pkt.fault_type);

        } else if (raw.len ==
                   sizeof(job_confirm_packet_t)) {
            // ── Job confirmation packet ───────────────
            job_confirm_packet_t job;
            memcpy(&job, raw.data,
                   sizeof(job_confirm_packet_t));

            // Verify CAFM marker byte
            if (job.packet_type != 0xCA) {
                ESP_LOGW(TAG,
                         "Unknown packet type 0x%02x",
                         job.packet_type);
                continue;
            }

            ESP_LOGI(TAG,
                     "RX job confirm: #%ld %s success=%d",
                     job.job_id,
                     job.job_number,
                     job.success);

            // Update HMI CAFM state
            hmi_lock();
            g_hmi.job_success    = job.success;
            g_hmi.job_id         = job.job_id;
            g_hmi.job_fault_type = job.fault_type;
            g_hmi.job_rms        = job.rms_at_fault;
            strncpy(g_hmi.job_number,
                    job.job_number,
                    sizeof(g_hmi.job_number) - 1);
            strncpy(g_hmi.job_created_at,
                    job.created_at,
                    sizeof(g_hmi.job_created_at) - 1);
            g_hmi.job_received = true;
            hmi_unlock();

        } else {
            ESP_LOGW(TAG,
                     "Unknown packet size %d — ignored",
                     raw.len);
        }
    }
}