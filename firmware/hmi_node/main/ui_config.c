#include "ui_config.h"
#include "hmi_state.h"

void ui_config_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1a1a2e), 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "CONFIGURATION");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *info = lv_label_create(parent);
    lv_label_set_text(info,
        "Detection Thresholds\n\n"
        "Normal:      5.5 - 7.0 A\n"
        "Warning:     7.0 - 7.8 A\n"
        "Mud Zone:    7.8 - 9.0 A\n"
        "Stall Zone:  > 9.0 A\n"
        "Dry Run:     < 1.5 A\n\n"
        "Calibrated at 200C load bank\n"
        "Baseline RMS: 6.65A");
    lv_obj_set_style_text_color(info, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_14, 0);
    lv_obj_align(info, LV_ALIGN_TOP_LEFT, 20, 45);

    lv_obj_t *sensor = lv_label_create(parent);
    lv_label_set_text(sensor,
        "Sensor Node MAC: 7C:DF:A1:E2:F2:30\n"
        "HMI MAC: 30:ED:A0:A0:6F:34\n"
        "Channel: 1  |  Rate: 10 Hz");
    lv_obj_set_style_text_color(sensor,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(sensor,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(sensor, LV_ALIGN_BOTTOM_LEFT, 10, -10);
}