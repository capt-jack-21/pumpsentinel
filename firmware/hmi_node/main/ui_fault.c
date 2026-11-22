#include "ui_fault.h"
#include "hmi_state.h"

static lv_obj_t *s_label_fault_type;
static lv_obj_t *s_label_detail;
static lv_obj_t *s_label_layers;
static lv_obj_t *s_label_time;

static const char *fault_detail(uint8_t ft)
{
    switch (ft) {
        case 1: return "Pump running without water.\nCheck water supply immediately.";
        case 2: return "High viscosity load detected.\nSediment or mud in pump chamber.";
        case 3: return "Rotor cannot turn — overload.\nCheck for mechanical blockage.";
        case 4: return "Pressure fluctuations detected.\nCheck inlet conditions.";
        default: return "System operating normally.";
    }
}

static lv_color_t fault_color(uint8_t ft)
{
    switch (ft) {
        case 1: return lv_color_hex(0xFF6B35);
        case 2: return lv_color_hex(0xFFCC00);
        case 3: return lv_color_hex(0xFF3333);
        case 4: return lv_color_hex(0xAA44FF);
        default: return lv_color_hex(0x00CC66);
    }
}

void ui_fault_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1a1a2e), 0);

    // Title
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "FAULT DETAIL");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF3333), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Fault type
    s_label_fault_type = lv_label_create(parent);
    lv_label_set_text(s_label_fault_type, "NONE");
    lv_obj_set_style_text_font(s_label_fault_type,
                                &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_label_fault_type,
                                 lv_color_hex(0x00CC66), 0);
    lv_obj_align(s_label_fault_type, LV_ALIGN_TOP_MID, 0, 40);

    // Detail text
    s_label_detail = lv_label_create(parent);
    lv_label_set_text(s_label_detail, "System operating normally.");
    lv_obj_set_style_text_color(s_label_detail,
                                 lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(s_label_detail,
                                &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(s_label_detail, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_label_detail, 420);
    lv_obj_align(s_label_detail, LV_ALIGN_TOP_MID, 0, 80);

    // Detection layers
    s_label_layers = lv_label_create(parent);
    lv_label_set_text(s_label_layers,
                      "L1: Z-Score          [  ok  ]\n"
                      "L2: Isolation Forest [  ok  ]\n"
                      "L3: Signature Match  [  ok  ]");
    lv_obj_set_style_text_color(s_label_layers,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(s_label_layers,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_layers, LV_ALIGN_TOP_LEFT, 20, 170);

    // Timestamp
    s_label_time = lv_label_create(parent);
    lv_label_set_text(s_label_time, "No fault recorded");
    lv_obj_set_style_text_color(s_label_time,
                                 lv_color_hex(0x666688), 0);
    lv_obj_set_style_text_font(s_label_time,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(s_label_time, LV_ALIGN_BOTTOM_LEFT, 10, -10);
}

void ui_fault_update(void)
{
    hmi_lock();
    uint8_t  fault = g_hmi.fault_type;
    bool     l1    = g_hmi.layer1_fired;
    bool     l2    = g_hmi.layer2_fired;
    bool     l3    = g_hmi.layer3_fired;
    uint32_t ts    = g_hmi.fault_timestamp / 1000;
    hmi_unlock();

    const char *names[] = {
        "NONE", "DRY RUN", "MUD", "STALL", "CAVITATION"
    };
    lv_label_set_text(s_label_fault_type,
                      fault < 5 ? names[fault] : "UNKNOWN");
    lv_obj_set_style_text_color(s_label_fault_type,
                                 fault_color(fault), 0);

    lv_label_set_text(s_label_detail, fault_detail(fault));

    char buf[128];
    snprintf(buf, sizeof(buf),
             "L1: Z-Score          [%s]\n"
             "L2: Isolation Forest [%s]\n"
             "L3: Signature Match  [%s]",
             l1 ? "FIRED" : "  ok ",
             l2 ? "FIRED" : "  ok ",
             l3 ? "FIRED" : "  ok ");
    lv_label_set_text(s_label_layers, buf);

    if (fault != 0) {
        char tbuf[32];
        snprintf(tbuf, sizeof(tbuf), "Last fault: %lus ago", ts);
        lv_label_set_text(s_label_time, tbuf);
    } else {
        lv_label_set_text(s_label_time, "No fault recorded");
    }
}