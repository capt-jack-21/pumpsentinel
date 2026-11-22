#include "ui_pumpiq.h"
#include "hmi_state.h"

static lv_obj_t *s_label_health_pct;
static lv_obj_t *s_bar_health;
static lv_obj_t *s_label_verdict;
static lv_obj_t *s_label_rms_val;
static lv_obj_t *s_label_cf_val;
static lv_obj_t *s_label_roc_val;
static lv_obj_t *s_label_anomaly_val;
static lv_obj_t *s_label_confidence;
static lv_obj_t *s_panel_verdict;

static const char *confidence_str(float anomaly)
{
    if (anomaly < 0.4f) return "HIGH";
    if (anomaly < 0.6f) return "MEDIUM";
    return "LOW";
}

static lv_color_t confidence_color(float anomaly)
{
    if (anomaly < 0.4f) return lv_color_hex(0x00CC66);
    if (anomaly < 0.6f) return lv_color_hex(0xFFA500);
    return lv_color_hex(0xFF3333);
}

static lv_color_t health_color(int pct)
{
    if (pct >= 75) return lv_color_hex(0x00CC66);
    if (pct >= 50) return lv_color_hex(0xFFA500);
    return lv_color_hex(0xFF3333);
}

void ui_pumpiq_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1a1a2e), 0);

    // ── Title ─────────────────────────────────────────
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "PumpIQ");
    lv_obj_set_style_text_color(title,
                                 lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(title,
                                &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 20, 8);

    lv_obj_t *subtitle = lv_label_create(parent);
    lv_label_set_text(subtitle, "AI-Powered Anomaly Detection");
    lv_obj_set_style_text_color(subtitle,
                                 lv_color_hex(0x666688), 0);
    lv_obj_set_style_text_font(subtitle,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_LEFT, 20, 38);

    // ── Divider ───────────────────────────────────────
    lv_obj_t *div1 = lv_obj_create(parent);
    lv_obj_set_size(div1, 760, 1);
    lv_obj_align(div1, LV_ALIGN_TOP_MID, 0, 58);
    lv_obj_set_style_bg_color(div1,
                               lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(div1, 0, 0);

    // ── Health Score label ────────────────────────────
    lv_obj_t *lbl_health_title = lv_label_create(parent);
    lv_label_set_text(lbl_health_title, "PUMP HEALTH SCORE");
    lv_obj_set_style_text_color(lbl_health_title,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_health_title,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_health_title,
                 LV_ALIGN_TOP_LEFT, 20, 68);

    s_label_health_pct = lv_label_create(parent);
    lv_label_set_text(s_label_health_pct, "100%");
    lv_obj_set_style_text_color(s_label_health_pct,
                                 lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_text_font(s_label_health_pct,
                                &lv_font_montserrat_24, 0);
    lv_obj_align(s_label_health_pct,
                 LV_ALIGN_TOP_RIGHT, -20, 62);

    // ── Health bar ────────────────────────────────────
    s_bar_health = lv_bar_create(parent);
    lv_obj_set_size(s_bar_health, 760, 18);
    lv_obj_align(s_bar_health, LV_ALIGN_TOP_MID, 0, 95);
    lv_bar_set_range(s_bar_health, 0, 100);
    lv_bar_set_value(s_bar_health, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_health,
                               lv_color_hex(0x2a2a4a), 0);
    lv_obj_set_style_bg_color(s_bar_health,
                               lv_color_hex(0x00CC66),
                               LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_bar_health, 4, 0);
    lv_obj_set_style_radius(s_bar_health, 4,
                             LV_PART_INDICATOR);

    // ── Divider ───────────────────────────────────────
    lv_obj_t *div2 = lv_obj_create(parent);
    lv_obj_set_size(div2, 760, 1);
    lv_obj_align(div2, LV_ALIGN_TOP_MID, 0, 124);
    lv_obj_set_style_bg_color(div2,
                               lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(div2, 0, 0);

    // ── Signal Features title ─────────────────────────
    lv_obj_t *lbl_feat = lv_label_create(parent);
    lv_label_set_text(lbl_feat, "SIGNAL FEATURES");
    lv_obj_set_style_text_color(lbl_feat,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_feat,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_feat, LV_ALIGN_TOP_LEFT, 20, 133);

    // ── RMS ───────────────────────────────────────────
    lv_obj_t *lbl_rms = lv_label_create(parent);
    lv_label_set_text(lbl_rms, "RMS Current:");
    lv_obj_set_style_text_color(lbl_rms,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(lbl_rms,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_rms, LV_ALIGN_TOP_LEFT, 20, 153);

    s_label_rms_val = lv_label_create(parent);
    lv_label_set_text(s_label_rms_val, "0.00 A");
    lv_obj_set_style_text_color(s_label_rms_val,
                                 lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(s_label_rms_val,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_rms_val,
                 LV_ALIGN_TOP_LEFT, 200, 153);

    // ── Crest Factor ──────────────────────────────────
    lv_obj_t *lbl_cf = lv_label_create(parent);
    lv_label_set_text(lbl_cf, "Crest Factor:");
    lv_obj_set_style_text_color(lbl_cf,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(lbl_cf,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cf, LV_ALIGN_TOP_LEFT, 20, 177);

    s_label_cf_val = lv_label_create(parent);
    lv_label_set_text(s_label_cf_val, "0.00");
    lv_obj_set_style_text_color(s_label_cf_val,
                                 lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(s_label_cf_val,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_cf_val,
                 LV_ALIGN_TOP_LEFT, 200, 177);

    // ── Rate of Change ────────────────────────────────
    lv_obj_t *lbl_roc = lv_label_create(parent);
    lv_label_set_text(lbl_roc, "Rate of Change:");
    lv_obj_set_style_text_color(lbl_roc,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(lbl_roc,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_roc, LV_ALIGN_TOP_LEFT, 20, 201);

    s_label_roc_val = lv_label_create(parent);
    lv_label_set_text(s_label_roc_val, "0.00 A/s");
    lv_obj_set_style_text_color(s_label_roc_val,
                                 lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(s_label_roc_val,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_roc_val,
                 LV_ALIGN_TOP_LEFT, 200, 201);

    // ── Anomaly Score ─────────────────────────────────
    lv_obj_t *lbl_anom = lv_label_create(parent);
    lv_label_set_text(lbl_anom, "Anomaly Score:");
    lv_obj_set_style_text_color(lbl_anom,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(lbl_anom,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_anom, LV_ALIGN_TOP_LEFT, 20, 225);

    s_label_anomaly_val = lv_label_create(parent);
    lv_label_set_text(s_label_anomaly_val, "0.00");
    lv_obj_set_style_text_color(s_label_anomaly_val,
                                 lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(s_label_anomaly_val,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_anomaly_val,
                 LV_ALIGN_TOP_LEFT, 200, 225);

    // ── Divider ───────────────────────────────────────
    lv_obj_t *div3 = lv_obj_create(parent);
    lv_obj_set_size(div3, 760, 1);
    lv_obj_align(div3, LV_ALIGN_TOP_MID, 0, 255);
    lv_obj_set_style_bg_color(div3,
                               lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(div3, 0, 0);

    // ── AI Decision panel ─────────────────────────────
    lv_obj_t *lbl_ai = lv_label_create(parent);
    lv_label_set_text(lbl_ai, "AI DECISION");
    lv_obj_set_style_text_color(lbl_ai,
                                 lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_ai,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_ai, LV_ALIGN_TOP_LEFT, 20, 264);

    // Confidence
    lv_obj_t *lbl_conf_title = lv_label_create(parent);
    lv_label_set_text(lbl_conf_title, "Confidence:");
    lv_obj_set_style_text_color(lbl_conf_title,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(lbl_conf_title,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_conf_title,
                 LV_ALIGN_TOP_LEFT, 20, 284);

    s_label_confidence = lv_label_create(parent);
    lv_label_set_text(s_label_confidence, "HIGH");
    lv_obj_set_style_text_color(s_label_confidence,
                                 lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_text_font(s_label_confidence,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_confidence,
                 LV_ALIGN_TOP_LEFT, 200, 284);

    // Verdict panel
    s_panel_verdict = lv_obj_create(parent);
    lv_obj_set_size(s_panel_verdict, 360, 50);
    lv_obj_align(s_panel_verdict,
                 LV_ALIGN_TOP_RIGHT, -20, 268);
    lv_obj_set_style_bg_color(s_panel_verdict,
                               lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_radius(s_panel_verdict, 8, 0);
    lv_obj_set_style_border_width(s_panel_verdict, 0, 0);

    s_label_verdict = lv_label_create(s_panel_verdict);
    lv_label_set_text(s_label_verdict, "PUMP HEALTHY");
    lv_obj_set_style_text_color(s_label_verdict,
                                 lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_label_verdict,
                                &lv_font_montserrat_16, 0);
    lv_obj_center(s_label_verdict);
}

void ui_pumpiq_update(void)
{
    hmi_lock();
    float   rms     = g_hmi.rms_current;
    float   cf      = g_hmi.crest_factor;
    float   roc     = g_hmi.rate_of_change;
    float   anomaly = g_hmi.anomaly_score;
    uint8_t fault   = g_hmi.fault_type;
    uint8_t state   = g_hmi.system_state;
    bool    connected = g_hmi.connected;
    hmi_unlock();

    char buf[32];

    // ── Health score — inverse of anomaly ─────────────
    int health_pct = connected
                     ? (int)((1.0f - anomaly) * 100.0f)
                     : 0;
    if (health_pct < 0)   health_pct = 0;
    if (health_pct > 100) health_pct = 100;

    // Override health for confirmed faults
    if (fault != 0 && connected) {
        switch (fault) {
            case 1: health_pct = 10; break; // dry run
            case 2: health_pct = 35; break; // mud
            case 3: health_pct = 5;  break; // stall
            default: break;
        }
    }

    // Health % label
    snprintf(buf, sizeof(buf), "%d%%", health_pct);
    lv_label_set_text(s_label_health_pct, buf);
    lv_obj_set_style_text_color(s_label_health_pct,
                                 health_color(health_pct), 0);

    // Health bar
    lv_bar_set_value(s_bar_health, health_pct, LV_ANIM_ON);
    lv_obj_set_style_bg_color(s_bar_health,
                               health_color(health_pct),
                               LV_PART_INDICATOR);

    // ── Features ──────────────────────────────────────
    snprintf(buf, sizeof(buf), "%.2f A", rms);
    lv_label_set_text(s_label_rms_val, buf);

    snprintf(buf, sizeof(buf), "%.2f", cf);
    lv_label_set_text(s_label_cf_val, buf);

    snprintf(buf, sizeof(buf), "%.1f A/s", roc);
    lv_label_set_text(s_label_roc_val, buf);

    snprintf(buf, sizeof(buf), "%.2f", anomaly);
    lv_label_set_text(s_label_anomaly_val, buf);

    // ── Confidence ────────────────────────────────────
    lv_label_set_text(s_label_confidence,
                      confidence_str(anomaly));
    lv_obj_set_style_text_color(s_label_confidence,
                                 confidence_color(anomaly), 0);

    // ── Verdict ───────────────────────────────────────
    if (!connected) {
        lv_obj_set_style_bg_color(s_panel_verdict,
                                   lv_color_hex(0x444444), 0);
        lv_label_set_text(s_label_verdict, "NO SIGNAL");
    } else if (fault == 0 && state == 0) {
        lv_obj_set_style_bg_color(s_panel_verdict,
                                   lv_color_hex(0x00CC66), 0);
        lv_label_set_text(s_label_verdict, "PUMP HEALTHY");
    } else if (state == 1) {
        lv_obj_set_style_bg_color(s_panel_verdict,
                                   lv_color_hex(0xFFA500), 0);
        lv_label_set_text(s_label_verdict, "ANOMALY DETECTED");
    } else {
        switch (fault) {
            case 1:
                lv_obj_set_style_bg_color(s_panel_verdict,
                    lv_color_hex(0xFF6B35), 0);
                lv_label_set_text(s_label_verdict,
                                  "DRY RUN DETECTED");
                break;
            case 2:
                lv_obj_set_style_bg_color(s_panel_verdict,
                    lv_color_hex(0xFFCC00), 0);
                lv_label_set_text(s_label_verdict,
                                  "MUD INGESTION");
                break;
            case 3:
                lv_obj_set_style_bg_color(s_panel_verdict,
                    lv_color_hex(0xFF3333), 0);
                lv_label_set_text(s_label_verdict,
                                  "ROTOR STALL");
                break;
            default:
                lv_obj_set_style_bg_color(s_panel_verdict,
                    lv_color_hex(0xFF3333), 0);
                lv_label_set_text(s_label_verdict,
                                  "FAULT DETECTED");
                break;
        }
    }
}