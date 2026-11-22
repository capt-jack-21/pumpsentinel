#include "http_alert.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "HTTP_ALERT";

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define WIFI_MAX_RETRY      10

static EventGroupHandle_t s_wifi_eg      = NULL;
static int                s_retry        = 0;
static bool               s_wifi_ready   = false;
static uint32_t           s_last_alert_ms = 0;
static char               s_response[2048] = {0};
static int                s_resp_len       = 0;

// ─── WiFi event handler ────────────────────────────────
static void wifi_event_handler(void *arg,
                                esp_event_base_t base,
                                int32_t id,
                                void *data)
{
    if (base == WIFI_EVENT &&
        id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (base == WIFI_EVENT &&
               id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry++;
            ESP_LOGW(TAG, "WiFi retry %d/%d",
                     s_retry, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_eg, WIFI_FAIL_BIT);
        }

    } else if (base == IP_EVENT &&
               id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt =
            (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "WiFi IP: " IPSTR,
                 IP2STR(&evt->ip_info.ip));
        s_retry     = 0;
        s_wifi_ready = true;
        xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
    }
}

// ─── HTTP event handler ────────────────────────────────
static esp_err_t http_event_handler(
    esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (s_resp_len + evt->data_len
                < (int)sizeof(s_response) - 1) {
            memcpy(s_response + s_resp_len,
                   evt->data, evt->data_len);
            s_resp_len += evt->data_len;
            s_response[s_resp_len] = '\0';
        }
    }
    return ESP_OK;
}

// ─── Init — connect to WiFi ────────────────────────────
void http_alert_init(void)
{
    s_wifi_eg = xEventGroupCreate();

    // sensor node WiFi shares the existing STA init
    // from espnow_tx — just register handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid               = SENSOR_WIFI_SSID,
            .password           = SENSOR_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,
                                         &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Connecting to WiFi: %s",
             SENSOR_WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_eg,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(15000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected");
    } else {
        ESP_LOGE(TAG, "WiFi failed — alerts disabled");
    }

    ESP_LOGI(TAG, "HTTP alert init done");
    ESP_LOGI(TAG, "Webhook: %s", STALL_WEBHOOK_URL);
}

// ─── Cooldown ──────────────────────────────────────────
bool http_alert_can_raise(void)
{
    if (!s_wifi_ready) return false;
    if (s_last_alert_ms == 0) return true;
    uint32_t now = esp_timer_get_time() / 1000;
    return (now - s_last_alert_ms) >= STALL_COOLDOWN_MS;
}

uint32_t http_alert_cooldown_remaining_ms(void)
{
    if (s_last_alert_ms == 0) return 0;
    uint32_t now     = esp_timer_get_time() / 1000;
    uint32_t elapsed = now - s_last_alert_ms;
    if (elapsed >= STALL_COOLDOWN_MS) return 0;
    return STALL_COOLDOWN_MS - elapsed;
}

// ─── Raise stall alert ─────────────────────────────────
job_result_t http_alert_raise_stall(float    rms_current,
                                     uint32_t uptime_ms)
{
    job_result_t result = {0};

    if (!s_wifi_ready) {
        snprintf(result.error, sizeof(result.error),
                 "WiFi not connected");
        return result;
    }

    char payload[512];
    snprintf(payload, sizeof(payload),
        "{"
        "\"fault\":\"ROTOR_STALL\","
        "\"rms_current\":%.2f,"
        "\"timestamp\":%lu,"
        "\"device_id\":\"PUMP-SENTINEL-01\","
        "\"uptime_ms\":%lu"
        "}",
        rms_current,
        (uint32_t)(esp_timer_get_time() / 1000000),
        uptime_ms);

    ESP_LOGI(TAG, "POST: %s", payload);

    memset(s_response, 0, sizeof(s_response));
    s_resp_len = 0;

    esp_http_client_config_t config = {
        .url           = STALL_WEBHOOK_URL,
        .method        = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms    = 15000,
        .skip_cert_common_name_check = true,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);
    esp_http_client_set_header(client,
        "Content-Type", "application/json");
    esp_http_client_set_post_field(client,
        payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status =
            esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP %d — %s", status, s_response);

        if (status == 200 && s_resp_len > 0) {
            cJSON *root = cJSON_Parse(s_response);
            if (root) {
                cJSON *ok  =
                    cJSON_GetObjectItem(root, "success");
                cJSON *jid =
                    cJSON_GetObjectItem(root, "job_id");
                cJSON *jno =
                    cJSON_GetObjectItem(root, "job_number");
                cJSON *cat =
                    cJSON_GetObjectItem(root, "created_at");

                if (cJSON_IsTrue(ok) &&
                    jid && cJSON_IsNumber(jid)) {
                    result.success = true;
                    result.job_id  = jid->valueint;
                    if (jno && cJSON_IsString(jno))
                        strncpy(result.job_number,
                                jno->valuestring,
                                sizeof(result.job_number)-1);
                    if (cat && cJSON_IsString(cat))
                        strncpy(result.created_at,
                                cat->valuestring,
                                sizeof(result.created_at)-1);
                    s_last_alert_ms =
                        esp_timer_get_time() / 1000;
                    ESP_LOGI(TAG, "Job created #%d %s",
                             result.job_id,
                             result.job_number);
                } else {
                    snprintf(result.error,
                             sizeof(result.error),
                             "No job_id in response");
                }
                cJSON_Delete(root);
            }
        } else {
            snprintf(result.error, sizeof(result.error),
                     "HTTP %d", status);
        }
    } else {
        snprintf(result.error, sizeof(result.error),
                 "%s", esp_err_to_name(err));
        ESP_LOGE(TAG, "HTTP failed: %s",
                 esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return result;
}