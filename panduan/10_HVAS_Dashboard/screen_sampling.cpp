/**
 * screen_sampling.c
 * SAMPLING SETUP page: sampling method, flow setpoint, duration, delay start,
 * filter ID, operator, plus SAVE / START buttons.
 *
 * Tapping a row is where you would open a keypad/roller to edit that value -
 * hook it up in row_event_cb() once you decide which input widget to use
 * (lv_keyboard, lv_roller, lv_spinbox, etc.).
 */
#include "ui_common.h"
#include "ui_screens.h"
#include "sampling_controller.h"

static void row_event_cb(lv_event_t * e)
{
    const char * field = (const char *)lv_event_get_user_data(e);
    LV_LOG_USER("Sampling row tapped: %s (open editor here)", field);
    /* TODO: open lv_roller / lv_spinbox / lv_keyboard depending on the field */
}

static void save_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    /* TODO: persist settings (e.g. write to NVS / SD config file) */
}

static void start_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    sampling_start();

    /* The controller owns the GPIO command and will reject a failed start.
     * Returning to Home immediately keeps the HMI flow simple; the Home
     * status/timer will show whether the run actually entered RUNNING. */
    ui_navigate_to(UI_PAGE_HOME);
}

lv_obj_t * screen_sampling_create(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, UI_COLOR_PAGE_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_subpage_topbar(scr, "SAMPLING SETUP", UI_PAGE_HOME);

    lv_obj_t * card = ui_create_card(scr, UI_HOR_RES - 32, UI_VER_RES - UI_SUBPAGE_TOPBAR_H - 32);
    lv_obj_set_pos(card, 16, UI_SUBPAGE_TOPBAR_H + 16);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * r1 = ui_create_settings_row(card, "Sampling Method", "TSP");
    lv_obj_add_event_cb(ui_settings_row_get_row(r1), row_event_cb, LV_EVENT_CLICKED, (void *)"sampling_method");

    lv_obj_t * r2 = ui_create_settings_row(card, "Flow Setpoint", "2000 LPM");
    lv_obj_add_event_cb(ui_settings_row_get_row(r2), row_event_cb, LV_EVENT_CLICKED, (void *)"flow_setpoint");

    lv_obj_t * r3 = ui_create_settings_row(card, "Sampling Duration", "24:00:00");
    lv_obj_add_event_cb(ui_settings_row_get_row(r3), row_event_cb, LV_EVENT_CLICKED, (void *)"duration");

    lv_obj_t * r4 = ui_create_settings_row(card, "Delay Start", "00:00:00");
    lv_obj_add_event_cb(ui_settings_row_get_row(r4), row_event_cb, LV_EVENT_CLICKED, (void *)"delay_start");

    lv_obj_t * r5 = ui_create_settings_row(card, "Filter ID", "GF250615001");
    lv_obj_add_event_cb(ui_settings_row_get_row(r5), row_event_cb, LV_EVENT_CLICKED, (void *)"filter_id");

    lv_obj_t * r6 = ui_create_settings_row(card, "Operator", "ENVILIFE");
    lv_obj_add_event_cb(ui_settings_row_get_row(r6), row_event_cb, LV_EVENT_CLICKED, (void *)"operator");
    lv_obj_set_style_border_width(ui_settings_row_get_row(r6), 0, 0); /* last row: no bottom divider */

    /* Spacer so the buttons sit at the bottom of the card */
    lv_obj_t * spacer = lv_obj_create(card);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_size(spacer, 1, 1);

    lv_obj_t * btn_row = lv_obj_create(card);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_size(btn_row, lv_pct(100), 56);
    lv_obj_set_layout(btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btn_row, 12, 0);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * save_btn = lv_btn_create(btn_row);
    lv_obj_set_style_bg_color(save_btn, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_border_color(save_btn, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(save_btn, 1, 0);
    lv_obj_set_style_shadow_width(save_btn, 0, 0);
    lv_obj_set_style_radius(save_btn, 8, 0);
    lv_obj_set_flex_grow(save_btn, 1);
    lv_obj_set_height(save_btn, 56);
    lv_obj_add_event_cb(save_btn, save_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, LV_SYMBOL_SAVE " SAVE");
    lv_obj_set_style_text_color(save_lbl, UI_COLOR_TEXT_DARK, 0);
    lv_obj_center(save_lbl);

    lv_obj_t * start_btn = ui_create_action_button(btn_row, LV_SYMBOL_PLAY, "START",
                                                    UI_COLOR_GREEN, UI_COLOR_GREEN_DARK);
    lv_obj_add_event_cb(start_btn, start_btn_cb, LV_EVENT_CLICKED, NULL);

    return scr;
}
