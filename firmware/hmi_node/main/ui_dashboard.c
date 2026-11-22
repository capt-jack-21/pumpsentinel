#include "ui_dashboard.h"
#include "hmi_state.h"

static lv_obj_t *s_panel_status;
static lv_obj_t *s_label_status;
static lv_obj_t *s_label_rms;
static lv_obj_t *s_label_cf;
static lv_obj_t *s_label_zscore;
static lv_obj_t *s_label_anomaly;
static lv_obj_t *s_label_fault;
static lv_obj_t *s_dot_l1;
static lv_obj_t *s_dot_l2;
static lv_obj_t *s_dot_l3;
static lv_obj_t *s_label_uptime;
static lv_obj_t *s_bar_rms;

static const char *fault_name(uint8_t ft)
{
    switch (ft) {
        case 1: return "DRY RUN";
        case 2: return "MUD INGESTION";
        case 3: return "ROTOR STALL";
        case 4: return "CAVITATION";
        default: return "NONE";
    }
}

static lv_color_t state_color(uint8_t state)
{
    switch (state) {
        case 1:  return lv_color_hex(0xFFA500);
        case 2:  return lv_color_hex(0xFF3333);
        default: return lv_color_hex(0x00CC66);
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

void ui_dashboard_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_pad_all(parent, 0, 0);

    // ── Title ─────────────────────────────────────────
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "PUMP HEALTH MONITOR");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    // ── Status banner ─────────────────────────────────
    s_panel_status = lv_obj_create(parent);
    lv_obj_set_size(s_panel_status, 760, 55);
    lv_obj_align(s_panel_status, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_bg_color(s_panel_status,
                               lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_radius(s_panel_status, 8, 0);
    lv_obj_set_style_border_width(s_panel_status, 0, 0);
    lv_obj_set_style_pad_all(s_panel_status, 0, 0);

    s_label_status = lv_label_create(s_panel_status);
    lv_label_set_text(s_label_status, "NORMAL");
    lv_obj_set_style_text_color(s_label_status,
                                 lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_label_status,
                                &lv_font_montserrat_24, 0);
    lv_obj_center(s_label_status);

    // ── Divider line ──────────────────────────────────
    lv_obj_t *div1 = lv_obj_create(parent);
    lv_obj_set_size(div1, 760, 1);
    lv_obj_align(div1, LV_ALIGN_TOP_MID, 0, 95);
    lv_obj_set_style_bg_color(div1, lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(div1, 0, 0);

    // ── Left column — RMS ─────────────────────────────
    lv_obj_t *lbl_rms_title = lv_label_create(parent);
    lv_label_set_text(lbl_rms_title, "RMS CURRENT");
    lv_obj_set_style_text_color(lbl_rms_title,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_rms_title,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_rms_title, LV_ALIGN_TOP_LEFT, 20, 105);

    s_label_rms = lv_label_create(parent);
    lv_label_set_text(s_label_rms, "0.00 A");
    lv_obj_set_style_text_color(s_label_rms,
                                 lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(s_label_rms,
                                &lv_font_montserrat_24, 0);
    lv_obj_align(s_label_rms, LV_ALIGN_TOP_LEFT, 20, 120);

    // ── Right column — Crest Factor ───────────────────
    lv_obj_t *lbl_cf_title = lv_label_create(parent);
    lv_label_set_text(lbl_cf_title, "CREST FACTOR");
    lv_obj_set_style_text_color(lbl_cf_title,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_cf_title,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_cf_title, LV_ALIGN_TOP_MID, 20, 105);

    s_label_cf = lv_label_create(parent);
    lv_label_set_text(s_label_cf, "0.00");
    lv_obj_set_style_text_color(s_label_cf,
                                 lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_label_cf,
                                &lv_font_montserrat_24, 0);
    lv_obj_align(s_label_cf, LV_ALIGN_TOP_MID, 20, 120);

    // ── RMS progress bar ──────────────────────────────
    s_bar_rms = lv_bar_create(parent);
    lv_obj_set_size(s_bar_rms, 760, 14);
    lv_obj_align(s_bar_rms, LV_ALIGN_TOP_MID, 0, 158);
    lv_bar_set_range(s_bar_rms, 0, 1000);
    lv_bar_set_value(s_bar_rms, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_rms,
                               lv_color_hex(0x2a2a4a), 0);
    lv_obj_set_style_bg_color(s_bar_rms,
                               lv_color_hex(0x00d4ff),
                               LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_bar_rms, 4, 0);
    lv_obj_set_style_radius(s_bar_rms, 4, LV_PART_INDICATOR);

    // ── Divider line 2 ────────────────────────────────
    lv_obj_t *div2 = lv_obj_create(parent);
    lv_obj_set_size(div2, 760, 1);
    lv_obj_align(div2, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_set_style_bg_color(div2, lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(div2, 0, 0);

    // ── Z-Score ───────────────────────────────────────
    lv_obj_t *lbl_z_title = lv_label_create(parent);
    lv_label_set_text(lbl_z_title, "Z-SCORE (L1)");
    lv_obj_set_style_text_color(lbl_z_title,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_z_title,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_z_title, LV_ALIGN_TOP_LEFT, 20, 190);

    s_label_zscore = lv_label_create(parent);
    lv_label_set_text(s_label_zscore, "0.00");
    lv_obj_set_style_text_color(s_label_zscore,
                                 lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_label_zscore,
                                &lv_font_montserrat_20, 0);
    lv_obj_align(s_label_zscore, LV_ALIGN_TOP_LEFT, 20, 206);

    // ── Anomaly Score ─────────────────────────────────
    lv_obj_t *lbl_if_title = lv_label_create(parent);
    lv_label_set_text(lbl_if_title, "ANOMALY (L2)");
    lv_obj_set_style_text_color(lbl_if_title,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_if_title,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_if_title, LV_ALIGN_TOP_MID, 20, 190);

    s_label_anomaly = lv_label_create(parent);
    lv_label_set_text(s_label_anomaly, "0.00");
    lv_obj_set_style_text_color(s_label_anomaly,
                                 lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_label_anomaly,
                                &lv_font_montserrat_20, 0);
    lv_obj_align(s_label_anomaly, LV_ALIGN_TOP_MID, 20, 206);

    // ── Fault type ────────────────────────────────────
    lv_obj_t *lbl_ft_title = lv_label_create(parent);
    lv_label_set_text(lbl_ft_title, "FAULT (L3)");
    lv_obj_set_style_text_color(lbl_ft_title,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_ft_title,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_ft_title, LV_ALIGN_TOP_RIGHT, -20, 190);

    s_label_fault = lv_label_create(parent);
    lv_label_set_text(s_label_fault, "NONE");
    lv_obj_set_style_text_color(s_label_fault,
                                 lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_text_font(s_label_fault,
                                &lv_font_montserrat_20, 0);
    lv_obj_align(s_label_fault, LV_ALIGN_TOP_RIGHT, -20, 206);

    // ── Divider line 3 ────────────────────────────────
    lv_obj_t *div3 = lv_obj_create(parent);
    lv_obj_set_size(div3, 760, 1);
    lv_obj_align(div3, LV_ALIGN_TOP_MID, 0, 242);
    lv_obj_set_style_bg_color(div3, lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(div3, 0, 0);

    // ── Detection layer dots ──────────────────────────
    lv_obj_t *lbl_layers = lv_label_create(parent);
    lv_label_set_text(lbl_layers, "DETECTION LAYERS:");
    lv_obj_set_style_text_color(lbl_layers,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_layers,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_layers, LV_ALIGN_TOP_LEFT, 20, 252);

    // L1 dot
    s_dot_l1 = lv_obj_create(parent);
    lv_obj_set_size(s_dot_l1, 16, 16);
    lv_obj_align(s_dot_l1, LV_ALIGN_TOP_LEFT, 170, 250);
    lv_obj_set_style_bg_color(s_dot_l1,
                               lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_border_width(s_dot_l1, 0, 0);
    lv_obj_set_style_radius(s_dot_l1, 8, 0);

    lv_obj_t *lbl_l1 = lv_label_create(parent);
    lv_label_set_text(lbl_l1, "L1");
    lv_obj_set_style_text_color(lbl_l1,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(lbl_l1,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_l1, LV_ALIGN_TOP_LEFT, 192, 252);

    // L2 dot
    s_dot_l2 = lv_obj_create(parent);
    lv_obj_set_size(s_dot_l2, 16, 16);
    lv_obj_align(s_dot_l2, LV_ALIGN_TOP_LEFT, 230, 250);
    lv_obj_set_style_bg_color(s_dot_l2,
                               lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_border_width(s_dot_l2, 0, 0);
    lv_obj_set_style_radius(s_dot_l2, 8, 0);

    lv_obj_t *lbl_l2 = lv_label_create(parent);
    lv_label_set_text(lbl_l2, "L2");
    lv_obj_set_style_text_color(lbl_l2,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(lbl_l2,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_l2, LV_ALIGN_TOP_LEFT, 252, 252);

    // L3 dot
    s_dot_l3 = lv_obj_create(parent);
    lv_obj_set_size(s_dot_l3, 16, 16);
    lv_obj_align(s_dot_l3, LV_ALIGN_TOP_LEFT, 290, 250);
    lv_obj_set_style_bg_color(s_dot_l3,
                               lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_border_width(s_dot_l3, 0, 0);
    lv_obj_set_style_radius(s_dot_l3, 8, 0);

    lv_obj_t *lbl_l3 = lv_label_create(parent);
    lv_label_set_text(lbl_l3, "L3");
    lv_obj_set_style_text_color(lbl_l3,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(lbl_l3,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_l3, LV_ALIGN_TOP_LEFT, 312, 252);

    // ── Uptime ────────────────────────────────────────
    s_label_uptime = lv_label_create(parent);
    lv_label_set_text(s_label_uptime, "Uptime: 0s  |  NO SIGNAL");
    lv_obj_set_style_text_color(s_label_uptime,
                                 lv_color_hex(0x555577), 0);
    lv_obj_set_style_text_font(s_label_uptime,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(s_label_uptime, LV_ALIGN_BOTTOM_LEFT, 10, -8);
}

void ui_dashboard_update(void)
{
    hmi_lock();
    float    rms      = g_hmi.rms_current;
    float    cf       = g_hmi.crest_factor;
    float    zscore   = g_hmi.zscore;
    float    anomaly  = g_hmi.anomaly_score;
    uint8_t  state    = g_hmi.system_state;
    uint8_t  fault    = g_hmi.fault_type;
    bool     l1       = g_hmi.layer1_fired;
    bool     l2       = g_hmi.layer2_fired;
    bool     l3       = g_hmi.layer3_fired;
    uint32_t uptime   = g_hmi.uptime_ms / 1000;
    bool     connected = g_hmi.connected;
    hmi_unlock();

    char buf[64];

    // ── Status banner ─────────────────────────────────
    if (!connected) {
        lv_obj_set_style_bg_color(s_panel_status,
                                   lv_color_hex(0x444444), 0);
        lv_label_set_text(s_label_status, "NO SIGNAL");
    } else {
        lv_obj_set_style_bg_color(s_panel_status,
                                   state_color(state), 0);
        lv_label_set_text(s_label_status,
            state == 0 ? "NORMAL" :
            state == 1 ? "WARNING" : "FAULT DETECTED");
    }

    // ── RMS ───────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%.2f A", rms);
    lv_label_set_text(s_label_rms, buf);
    lv_bar_set_value(s_bar_rms, (int)(rms * 100), LV_ANIM_ON);

    // ── Crest Factor ──────────────────────────────────
    snprintf(buf, sizeof(buf), "%.2f", cf);
    lv_label_set_text(s_label_cf, buf);

    // ── Z-Score ───────────────────────────────────────
    snprintf(buf, sizeof(buf), "%.2f", zscore);
    lv_label_set_text(s_label_zscore, buf);
    lv_obj_set_style_text_color(s_label_zscore,
        l1 ? lv_color_hex(0xFF3333) : lv_color_hex(0xFFFFFF), 0);

    // ── Anomaly Score ─────────────────────────────────
    snprintf(buf, sizeof(buf), "%.2f", anomaly);
    lv_label_set_text(s_label_anomaly, buf);
    lv_obj_set_style_text_color(s_label_anomaly,
        l2 ? lv_color_hex(0xFF3333) : lv_color_hex(0xFFFFFF), 0);

    // ── Fault ─────────────────────────────────────────
    lv_label_set_text(s_label_fault, fault_name(fault));
    lv_obj_set_style_text_color(s_label_fault,
                                 fault_color(fault), 0);

    // ── Layer dots ────────────────────────────────────
    lv_obj_set_style_bg_color(s_dot_l1,
        l1 ? lv_color_hex(0xFF3333) : lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_bg_color(s_dot_l2,
        l2 ? lv_color_hex(0xFF3333) : lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_bg_color(s_dot_l3,
        l3 ? lv_color_hex(0xFF3333) : lv_color_hex(0x00CC66), 0);

    // ── Uptime ────────────────────────────────────────
    snprintf(buf, sizeof(buf), "Uptime: %lus  |  %s",
             uptime,
             connected ? "CONNECTED" : "NO SIGNAL");
    lv_label_set_text(s_label_uptime, buf);
}