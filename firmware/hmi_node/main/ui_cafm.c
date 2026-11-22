 #include "ui_cafm.h"
#include "hmi_state.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_panel_fault;
static lv_obj_t *s_label_fault_detail;
static lv_obj_t *s_panel_job;
static lv_obj_t *s_label_job_status;
static lv_obj_t *s_label_job_id;
static lv_obj_t *s_label_job_ref;
static lv_obj_t *s_label_job_site;
static lv_obj_t *s_label_job_created;
static lv_obj_t *s_label_job_simpro;
static lv_obj_t *s_panel_pipeline;
static lv_obj_t *s_label_pipeline;

void ui_cafm_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent,
                               lv_color_hex(0x1a1a2e), 0);

    // ── Title ─────────────────────────────────────────
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "CAFM INTEGRATION");
    lv_obj_set_style_text_color(title,
                                 lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(title,
                                &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 20, 8);

    lv_obj_t *subtitle = lv_label_create(parent);
    lv_label_set_text(subtitle, "Powered by Simpro");
    lv_obj_set_style_text_color(subtitle,
                                 lv_color_hex(0x555577), 0);
    lv_obj_set_style_text_font(subtitle,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_RIGHT, -20, 12);

    // ── Divider ───────────────────────────────────────
    lv_obj_t *div = lv_obj_create(parent);
    lv_obj_set_size(div, 760, 1);
    lv_obj_align(div, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_bg_color(div,
                               lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(div, 0, 0);

    // ── Fault event panel ─────────────────────────────
    s_panel_fault = lv_obj_create(parent);
    lv_obj_set_size(s_panel_fault, 760, 62);
    lv_obj_align(s_panel_fault,
                 LV_ALIGN_TOP_MID, 0, 44);
    lv_obj_set_style_bg_color(s_panel_fault,
                               lv_color_hex(0x2A0000), 0);
    lv_obj_set_style_border_color(s_panel_fault,
                                   lv_color_hex(0xFF3333),
                                   0);
    lv_obj_set_style_border_width(s_panel_fault, 1, 0);
    lv_obj_set_style_radius(s_panel_fault, 8, 0);

    lv_obj_t *fault_icon = lv_label_create(s_panel_fault);
    lv_label_set_text(fault_icon, "ROTOR STALL DETECTED");
    lv_obj_set_style_text_color(fault_icon,
                                 lv_color_hex(0xFF3333), 0);
    lv_obj_set_style_text_font(fault_icon,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(fault_icon, LV_ALIGN_TOP_LEFT, 12, 8);

    s_label_fault_detail = lv_label_create(s_panel_fault);
    lv_label_set_text(s_label_fault_detail,
                      "RMS Current: 9.31A   "
                      "Confirmed: 3s debounce   "
                      "Uptime: 120s");
    lv_obj_set_style_text_color(s_label_fault_detail,
                                 lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(s_label_fault_detail,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(s_label_fault_detail,
                 LV_ALIGN_TOP_LEFT, 12, 32);

    // ── Job created panel ─────────────────────────────
    s_panel_job = lv_obj_create(parent);
    lv_obj_set_size(s_panel_job, 760, 185);
    lv_obj_align(s_panel_job,
                 LV_ALIGN_TOP_MID, 0, 116);
    lv_obj_set_style_bg_color(s_panel_job,
                               lv_color_hex(0x0A2A0A), 0);
    lv_obj_set_style_border_color(s_panel_job,
                                   lv_color_hex(0x00CC66),
                                   0);
    lv_obj_set_style_border_width(s_panel_job, 1, 0);
    lv_obj_set_style_radius(s_panel_job, 8, 0);

    s_label_job_status = lv_label_create(s_panel_job);
    lv_label_set_text(s_label_job_status,
                      "CAFM Job Created Successfully");
    lv_obj_set_style_text_color(s_label_job_status,
                                 lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_text_font(s_label_job_status,
                                &lv_font_montserrat_16, 0);
    lv_obj_align(s_label_job_status,
                 LV_ALIGN_TOP_LEFT, 12, 10);

    // Job fields
    const char *field_labels[] = {
        "Job ID:", "Reference:", "Site:", "Created:", "Status:"
    };
    const char *field_values[] = {
        "#42", "RPM-2026-001", "Building 1",
        "2026-06-27  09:14:22", "Submitted"
    };

    lv_obj_t **field_objs[] = {
        &s_label_job_id, &s_label_job_ref,
        &s_label_job_site, &s_label_job_created,
        &s_label_job_simpro
    };

    for (int i = 0; i < 5; i++) {
        lv_obj_t *lbl_key = lv_label_create(s_panel_job);
        lv_label_set_text(lbl_key, field_labels[i]);
        lv_obj_set_style_text_color(lbl_key,
                                     lv_color_hex(0x888888),
                                     0);
        lv_obj_set_style_text_font(lbl_key,
                                    &lv_font_montserrat_12,
                                    0);
        lv_obj_align(lbl_key,
                     LV_ALIGN_TOP_LEFT,
                     12, 44 + i * 28);

        *field_objs[i] = lv_label_create(s_panel_job);
        lv_label_set_text(*field_objs[i], field_values[i]);
        lv_obj_set_style_text_color(*field_objs[i],
            i == 4
                ? lv_color_hex(0x00CC66)
                : lv_color_hex(0xFFFFFF),
            0);
        lv_obj_set_style_text_font(*field_objs[i],
                                    &lv_font_montserrat_12,
                                    0);
        lv_obj_align(*field_objs[i],
                     LV_ALIGN_TOP_LEFT,
                     130, 44 + i * 28);
    }

    // ── Pipeline status panel ─────────────────────────
    s_panel_pipeline = lv_obj_create(parent);
    lv_obj_set_size(s_panel_pipeline, 760, 55);
    lv_obj_align(s_panel_pipeline,
                 LV_ALIGN_TOP_MID, 0, 310);
    lv_obj_set_style_bg_color(s_panel_pipeline,
                               lv_color_hex(0x0d0d2a), 0);
    lv_obj_set_style_border_color(s_panel_pipeline,
                                   lv_color_hex(0x333355),
                                   0);
    lv_obj_set_style_border_width(s_panel_pipeline, 1, 0);
    lv_obj_set_style_radius(s_panel_pipeline, 8, 0);

    s_label_pipeline = lv_label_create(s_panel_pipeline);
    lv_label_set_text(s_label_pipeline,
                      "Pipeline:   "
                      "PumpSentinel  ->  "
                      "n8n Workflow  ->  "
                      "Simpro API  ->  "
                      "Job Created");
    lv_obj_set_style_text_color(s_label_pipeline,
                                 lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_text_font(s_label_pipeline,
                                &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_pipeline,
                 LV_ALIGN_CENTER, 0, 0);

    // ── Bottom note ───────────────────────────────────
    lv_obj_t *note = lv_label_create(parent);
    lv_label_set_text(note,
                      "Automated alert pipeline — "
                      "fault detected to work order "
                      "in under 5 seconds");
    lv_obj_set_style_text_color(note,
                                 lv_color_hex(0x555577), 0);
    lv_obj_set_style_text_font(note,
                                &lv_font_montserrat_12, 0);
    lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -8);
}

void ui_cafm_update(void)
{
    hmi_lock();
    bool     received = g_hmi.job_received;
    bool     success  = g_hmi.job_success;
    int32_t  job_id   = g_hmi.job_id;
    float    rms      = g_hmi.job_rms;
    uint32_t uptime   = g_hmi.uptime_ms;
    char     job_num[32];
    char     created[24];
    strncpy(job_num, g_hmi.job_number,
            sizeof(job_num) - 1);
    strncpy(created, g_hmi.job_created_at,
            sizeof(created) - 1);
    hmi_unlock();

    if (!received) return; // keep mock data until real arrives

    char buf[64];

    // Fault detail
    snprintf(buf, sizeof(buf),
             "RMS Current: %.2fA   "
             "Confirmed: 3s debounce   "
             "Uptime: %lus",
             rms, uptime / 1000);
    lv_label_set_text(s_label_fault_detail, buf);

    if (success) {
        // Green job panel
        lv_obj_set_style_bg_color(s_panel_job,
            lv_color_hex(0x0A2A0A), 0);
        lv_obj_set_style_border_color(s_panel_job,
            lv_color_hex(0x00CC66), 0);
        lv_label_set_text(s_label_job_status,
                          "CAFM Job Created Successfully");
        lv_obj_set_style_text_color(s_label_job_status,
            lv_color_hex(0x00CC66), 0);

        snprintf(buf, sizeof(buf), "#%ld", job_id);
        lv_label_set_text(s_label_job_id, buf);

        lv_label_set_text(s_label_job_ref,
                          strlen(job_num) > 0
                              ? job_num : "—");

        // Trim timestamp
        char ts[20] = "--";
        if (strlen(created) >= 19) {
            strncpy(ts, created, 19);
            ts[19] = '\0';
        }
        lv_label_set_text(s_label_job_created, ts);

    } else {
        // Red — failed
        lv_obj_set_style_bg_color(s_panel_job,
            lv_color_hex(0x2A0A00), 0);
        lv_obj_set_style_border_color(s_panel_job,
            lv_color_hex(0xFF3333), 0);
        lv_label_set_text(s_label_job_status,
                          "Job Creation Failed");
        lv_obj_set_style_text_color(s_label_job_status,
            lv_color_hex(0xFF3333), 0);
        lv_label_set_text(s_label_job_id, "—");
        lv_label_set_text(s_label_job_ref, "—");
        lv_label_set_text(s_label_job_created, "—");
    }
}