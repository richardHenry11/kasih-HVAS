/**
 * screen_calibration.c
 * CALIBRATION page (reached from SYSTEM): Flow/Temperature/Humidity/Pressure/
 * Wind/Factory calibration entries, plus a "CALIBRATION INFO" button.
 */
#include "ui_common.h"
#include "ui_screens.h"

static void cal_row_cb(lv_event_t * e)
{
    const char * which = (const char *)lv_event_get_user_data(e);
    LV_LOG_USER("Open calibration wizard: %s", which);
    /* TODO: open the relevant step-by-step calibration flow/screen */
}

static void info_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    LV_LOG_USER("Show calibration info dialog (lv_msgbox)");
}

lv_obj_t * screen_calibration_create(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, UI_COLOR_PAGE_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_subpage_topbar(scr, "CALIBRATION", UI_PAGE_SYSTEM);

    lv_obj_t * card = ui_create_card(scr, UI_HOR_RES - 32, UI_VER_RES - UI_SUBPAGE_TOPBAR_H - 32);
    lv_obj_set_pos(card, 16, UI_SUBPAGE_TOPBAR_H + 16);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    const char * items[] = {
        "Flow Calibration", "Temperature Calibration", "Humidity Calibration",
        "Pressure Calibration", "Wind Calibration", "Factory Calibration",
    };
    for (int i = 0; i < 6; i++) {
        lv_obj_t * val = ui_create_settings_row(card, items[i], "");
        lv_obj_add_flag(val, LV_OBJ_FLAG_HIDDEN); /* no trailing value shown in the mock-up */
        lv_obj_t * row = ui_settings_row_get_row(val);
        lv_obj_add_event_cb(row, cal_row_cb, LV_EVENT_CLICKED, (void *)items[i]);
        if (i == 5) lv_obj_set_style_border_width(row, 0, 0);
    }

    lv_obj_t * spacer = lv_obj_create(card);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_size(spacer, 1, 1);

    lv_obj_t * info_btn = lv_btn_create(card);
    lv_obj_set_style_bg_opa(info_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(info_btn, UI_COLOR_BLUE, 0);
    lv_obj_set_style_border_width(info_btn, 1, 0);
    lv_obj_set_style_shadow_width(info_btn, 0, 0);
    lv_obj_set_style_radius(info_btn, 8, 0);
    lv_obj_set_size(info_btn, lv_pct(100), 48);
    lv_obj_add_event_cb(info_btn, info_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * info_lbl = lv_label_create(info_btn);
    lv_label_set_text(info_lbl, LV_SYMBOL_WARNING " CALIBRATION INFO");
    lv_obj_set_style_text_color(info_lbl, UI_COLOR_BLUE, 0);
    lv_obj_center(info_lbl);

    return scr;
}
