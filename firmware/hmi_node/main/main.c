#include "waveshare_rgb_lcd_port.h"
#include "lvgl_port.h"
#include "espnow_rx.h"
#include "hmi_state.h"
#include "ui_dashboard.h"
#include "ui_fault.h"
#include "ui_config.h"
#include "ui_pumpiq.h"
#include "ui_cafm.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

hmi_data_t        g_hmi       = {0};
SemaphoreHandle_t g_hmi_mutex = NULL;

static lv_obj_t *s_tabview;
static lv_obj_t *s_tab_dashboard;
static lv_obj_t *s_tab_fault;
static lv_obj_t *s_tab_config;
static lv_obj_t *s_tab_pumpiq;
static lv_obj_t *s_tab_cafm;

static void ui_update_cb(lv_timer_t *timer)
{
    ui_dashboard_update();
    ui_fault_update();
    ui_pumpiq_update();
    ui_cafm_update();
}

static void ui_task(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(500));

    if (lvgl_port_lock(-1)) {
        s_tabview = lv_tabview_create(lv_scr_act(),
                                      LV_DIR_TOP, 40);
        lv_obj_set_style_bg_color(s_tabview,
                                   lv_color_hex(0x1a1a2e),
                                   0);

        lv_obj_t *tab_bar =
            lv_tabview_get_tab_btns(s_tabview);
        lv_obj_set_style_bg_color(tab_bar,
                                   lv_color_hex(0x0d0d1a),
                                   0);

        s_tab_dashboard = lv_tabview_add_tab(s_tabview,
                                              "Dashboard");
        s_tab_fault     = lv_tabview_add_tab(s_tabview,
                                              "Fault");
        s_tab_config    = lv_tabview_add_tab(s_tabview,
                                              "Config");
        s_tab_pumpiq    = lv_tabview_add_tab(s_tabview,
                                              "PumpIQ");
        s_tab_cafm      = lv_tabview_add_tab(s_tabview,
                                              "CAFM");

        ui_dashboard_create(s_tab_dashboard);
        ui_fault_create(s_tab_fault);
        ui_config_create(s_tab_config);
        ui_pumpiq_create(s_tab_pumpiq);
        ui_cafm_create(s_tab_cafm);

        lv_timer_create(ui_update_cb, 200, NULL);

        lvgl_port_unlock();
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    g_hmi_mutex = xSemaphoreCreateMutex();

    waveshare_esp32_s3_rgb_lcd_init();

    xTaskCreatePinnedToCore(ui_task, "ui_task",
                             8192, NULL, 5, NULL, 0);

    espnow_rx_init();
    xTaskCreatePinnedToCore(espnow_rx_task, "espnow_rx",
                             ESPNOW_RX_TASK_STACK,
                             NULL,
                             ESPNOW_RX_TASK_PRIORITY,
                             NULL,
                             ESPNOW_RX_TASK_CORE);
}