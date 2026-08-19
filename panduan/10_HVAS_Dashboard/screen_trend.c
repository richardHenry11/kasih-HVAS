/**
 * screen_trend.c
 * TREND page: live flow-rate line chart with AVG/MIN/MAX summary and a
 * time-range selector (10/30/60/120 MIN).
 */
#include "ui_common.h"
#include "ui_screens.h"
#include <stdio.h>
#include <limits.h>

#define TREND_POINTS 60   /* one point per minute, matches the "Live (60 min)" mock-up */

static lv_obj_t * s_chart;
static lv_chart_series_t * s_series;
static lv_obj_t * s_lbl_range_caption;
static lv_obj_t * s_lbl_avg;
static lv_obj_t * s_lbl_min;
static lv_obj_t * s_lbl_max;
static lv_obj_t * s_range_buttons[4];
static int s_visible_window = 60; /* currently selected MIN range */

static void range_btn_cb(lv_event_t * e)
{
    int minutes = (int)(uintptr_t)lv_event_get_user_data(e);
    s_visible_window = minutes;

    char buf[24];
    snprintf(buf, sizeof(buf), "Live (%d min)", minutes);
    lv_label_set_text(s_lbl_range_caption, buf);

    for (int i = 0; i < 4; i++) {
        bool active = (lv_obj_get_user_data(s_range_buttons[i]) == (void *)(uintptr_t)minutes);
        lv_obj_set_style_bg_color(s_range_buttons[i], active ? UI_COLOR_BLUE : UI_COLOR_CARD_BG, 0);
        lv_obj_t * lbl = lv_obj_get_child(s_range_buttons[i], 0);
        lv_obj_set_style_text_color(lbl, active ? UI_COLOR_WHITE : UI_COLOR_TEXT_DARK, 0);
    }
    /* NOTE: this UI keeps a fixed 60-point ring buffer; swap chart point count
     * here if you want a genuinely different sample density per range. */
}

static lv_obj_t * build_range_btn(lv_obj_t * parent, const char * text, int minutes, bool active)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_style_bg_color(btn, active ? UI_COLOR_BLUE : UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_border_color(btn, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, 44);
    lv_obj_set_user_data(btn, (void *)(uintptr_t)minutes);
    lv_obj_add_event_cb(btn, range_btn_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)minutes);

    lv_obj_t * lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, active ? UI_COLOR_WHITE : UI_COLOR_TEXT_DARK, 0);
    lv_obj_center(lbl);

    return btn;
}

static lv_obj_t * build_summary_tile(lv_obj_t * parent, const char * caption, const char * value)
{
    lv_obj_t * tile = ui_create_card(parent, 0, 64);
    lv_obj_set_flex_grow(tile, 1);
    lv_obj_set_layout(tile, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * caption_lbl = lv_label_create(tile);
    lv_label_set_text(caption_lbl, caption);
    lv_obj_set_style_text_font(caption_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(caption_lbl, UI_COLOR_TEXT_MUTED, 0);

    lv_obj_t * value_lbl = lv_label_create(tile);
    lv_label_set_text(value_lbl, value);
    lv_obj_set_style_text_font(value_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(value_lbl, UI_COLOR_TEXT_DARK, 0);

    return value_lbl;
}

lv_obj_t * screen_trend_create(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, UI_COLOR_PAGE_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_subpage_topbar(scr, "TREND", UI_PAGE_HOME);

    lv_obj_t * content = lv_obj_create(scr);
    lv_obj_remove_style_all(content);
    lv_obj_set_pos(content, 16, UI_SUBPAGE_TOPBAR_H + 16);
    lv_obj_set_size(content, UI_HOR_RES - 32, UI_VER_RES - UI_SUBPAGE_TOPBAR_H - 32);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 10, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* Chart card */
    lv_obj_t * chart_card = ui_create_card(content, lv_pct(100), 230);
    lv_obj_set_layout(chart_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(chart_card, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * header = lv_obj_create(chart_card);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text(title, LV_SYMBOL_LOOP " Flow Rate (LPM)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_BLUE, 0);

    s_lbl_range_caption = lv_label_create(header);
    lv_label_set_text(s_lbl_range_caption, "Live (60 min)");
    lv_obj_set_style_text_font(s_lbl_range_caption, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_lbl_range_caption, UI_COLOR_TEXT_MUTED, 0);

    s_chart = lv_chart_create(chart_card);
    lv_obj_set_size(s_chart, lv_pct(100), 160);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 2500);
    lv_chart_set_point_count(s_chart, TREND_POINTS);
    lv_chart_set_div_line_count(s_chart, 5, 0);
    lv_obj_set_style_bg_color(s_chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_border_width(s_chart, 0, 0);
    lv_obj_set_style_line_color(s_chart, UI_COLOR_GREEN, LV_PART_ITEMS);
    lv_obj_set_style_size(s_chart, 0, LV_PART_INDICATOR); /* hide point markers, mock-up shows a clean line */

    s_series = lv_chart_add_series(s_chart, UI_COLOR_GREEN, LV_CHART_AXIS_PRIMARY_Y);
    for (int i = 0; i < TREND_POINTS; i++) {
        lv_chart_set_next_value(s_chart, s_series, 2000);
    }

    /* Summary tiles: AVG / MIN / MAX */
    lv_obj_t * summary_row = lv_obj_create(content);
    lv_obj_remove_style_all(summary_row);
    lv_obj_set_size(summary_row, lv_pct(100), 64);
    lv_obj_set_layout(summary_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(summary_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(summary_row, 10, 0);
    lv_obj_clear_flag(summary_row, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_avg = build_summary_tile(summary_row, "AVG", "2003 LPM");
    s_lbl_min = build_summary_tile(summary_row, "MIN", "1982 LPM");
    s_lbl_max = build_summary_tile(summary_row, "MAX", "2025 LPM");

    /* Range selector */
    lv_obj_t * range_row = lv_obj_create(content);
    lv_obj_remove_style_all(range_row);
    lv_obj_set_size(range_row, lv_pct(100), 44);
    lv_obj_set_layout(range_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(range_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(range_row, 10, 0);
    lv_obj_clear_flag(range_row, LV_OBJ_FLAG_SCROLLABLE);

    s_range_buttons[0] = build_range_btn(range_row, "10 MIN", 10, false);
    s_range_buttons[1] = build_range_btn(range_row, "30 MIN", 30, false);
    s_range_buttons[2] = build_range_btn(range_row, "60 MIN", 60, true);
    s_range_buttons[3] = build_range_btn(range_row, "120 MIN", 120, false);

    return scr;
}

/* ------------------------------------------------------------------------ */
/*  Live-data update API                                                     */
/* ------------------------------------------------------------------------ */
void screen_trend_add_sample(float lpm)
{
    if (!s_chart) return;

    lv_chart_set_next_value(s_chart, s_series, (lv_coord_t)lpm);

    /* Recompute AVG/MIN/MAX over the currently plotted points */
    lv_coord_t * pts = lv_chart_get_y_array(s_chart, s_series);
    uint16_t cnt = lv_chart_get_point_count(s_chart);
    long sum = 0;
    lv_coord_t vmin = LV_COORD_MAX, vmax = LV_COORD_MIN;
    int valid = 0;
    for (uint16_t i = 0; i < cnt; i++) {
        if (pts[i] == LV_CHART_POINT_NONE) continue;
        sum += pts[i];
        if (pts[i] < vmin) vmin = pts[i];
        if (pts[i] > vmax) vmax = pts[i];
        valid++;
    }
    if (valid == 0) return;

    char buf[24];
    snprintf(buf, sizeof(buf), "%ld LPM", sum / valid);
    lv_label_set_text(s_lbl_avg, buf);
    snprintf(buf, sizeof(buf), "%d LPM", vmin);
    lv_label_set_text(s_lbl_min, buf);
    snprintf(buf, sizeof(buf), "%d LPM", vmax);
    lv_label_set_text(s_lbl_max, buf);
}
