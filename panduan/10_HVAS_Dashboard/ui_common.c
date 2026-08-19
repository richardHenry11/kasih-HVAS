/**
 * ui_common.c
 * Implementation of shared widget helpers. See ui_common.h for docs.
 */
#include "ui_common.h"
#include <stdio.h>

/* Forward declaration from ui.c (the navigation/screen-manager) */
extern void ui_screen_manager_navigate(ui_page_id_t page);

/* Runtime labels for the Home top bar. The Home screen is created once, so
 * keeping these pointers is safe and lets controllers push live values. */
static lv_obj_t * s_home_date_lbl = NULL;
static lv_obj_t * s_home_time_lbl = NULL;
static lv_obj_t * s_home_lat_lbl = NULL;
static lv_obj_t * s_home_lng_lbl = NULL;

void ui_navigate_to(ui_page_id_t page)
{
    ui_screen_manager_navigate(page);
}

/* ------------------------------------------------------------------------ */
/*  Sidebar (Home screen only)                                              */
/* ------------------------------------------------------------------------ */
typedef struct {
    const char * symbol;
    const char * label;
    ui_page_id_t page;
} nav_item_t;

static const nav_item_t s_nav_items[] = {
    { LV_SYMBOL_HOME,     "HOME",   UI_PAGE_HOME   },
    { LV_SYMBOL_SETTINGS, "SYSTEM", UI_PAGE_SYSTEM },
};
#define NAV_ITEM_COUNT (sizeof(s_nav_items) / sizeof(s_nav_items[0]))

static void nav_btn_event_cb(lv_event_t * e)
{
    ui_page_id_t page = (ui_page_id_t)(uintptr_t)lv_event_get_user_data(e);
    ui_navigate_to(page);
}

lv_obj_t * ui_create_sidebar(lv_obj_t * parent, ui_page_id_t active_page)
{
    lv_obj_t * sidebar = lv_obj_create(parent);
    lv_obj_remove_style_all(sidebar);
    lv_obj_set_size(sidebar, UI_SIDEBAR_WIDTH, UI_VER_RES);
    lv_obj_set_pos(sidebar, 0, 0);
    lv_obj_set_style_bg_color(sidebar, UI_COLOR_NAVY_DARK, 0);
    lv_obj_set_style_bg_opa(sidebar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(sidebar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(sidebar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sidebar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(sidebar, 8, 0);
    lv_obj_set_style_pad_row(sidebar, 4, 0);

    for (uint32_t i = 0; i < NAV_ITEM_COUNT; i++) {
        const nav_item_t * item = &s_nav_items[i];
        bool is_active = (item->page == active_page);

        lv_obj_t * btn = lv_obj_create(sidebar);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, UI_SIDEBAR_WIDTH - 12, 64);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_bg_opa(btn, is_active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(btn, UI_COLOR_TEAL_ACTIVE, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)item->page);

        lv_obj_t * icon = lv_label_create(btn);
        lv_label_set_text(icon, item->symbol);
        lv_obj_set_style_text_color(icon, UI_COLOR_WHITE, 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, item->label);
        lv_obj_set_style_text_color(label, UI_COLOR_WHITE, 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    }

    return sidebar;
}

/* ------------------------------------------------------------------------ */
/*  Home screen top bar                                                     */
/* ------------------------------------------------------------------------ */
lv_obj_t * ui_create_home_topbar(lv_obj_t * parent)
{
    lv_obj_t * bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, UI_HOR_RES - UI_SIDEBAR_WIDTH, UI_TOPBAR_HEIGHT);
    lv_obj_set_pos(bar, UI_SIDEBAR_WIDTH, 0);
    lv_obj_set_style_bg_color(bar, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(bar, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(bar, 16, 0);

    /* Brand + title block */
    lv_obj_t * brand = lv_obj_create(bar);
    lv_obj_remove_style_all(brand);
    lv_obj_set_size(brand, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(brand, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(brand, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * title = lv_label_create(brand);
    lv_label_set_text(title, "envilife | HVAS TSP");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_DARK, 0);

    lv_obj_t * subtitle = lv_label_create(brand);
    lv_label_set_text(subtitle, "HIGH VOLUME AIR SAMPLER");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(subtitle, UI_COLOR_TEXT_MUTED, 0);

    /* Date / time block (create as named labels so the caller can update them) */
    lv_obj_t * dt_box = lv_obj_create(bar);
    lv_obj_remove_style_all(dt_box);
    lv_obj_set_size(dt_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(dt_box, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dt_box, LV_FLEX_FLOW_COLUMN);

    s_home_date_lbl = lv_label_create(dt_box);
    lv_label_set_text(s_home_date_lbl, "--");
    lv_obj_set_style_text_font(s_home_date_lbl, &lv_font_montserrat_14, 0);

    s_home_time_lbl = lv_label_create(dt_box);
    lv_label_set_text(s_home_time_lbl, "--:--:--");
    lv_obj_set_style_text_font(s_home_time_lbl, &lv_font_montserrat_14, 0);

    /* GPS block */
    lv_obj_t * gps_box = lv_obj_create(bar);
    lv_obj_remove_style_all(gps_box);
    lv_obj_set_size(gps_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(gps_box, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(gps_box, LV_FLEX_FLOW_COLUMN);

    s_home_lat_lbl = lv_label_create(gps_box);
    lv_label_set_text(s_home_lat_lbl, "Lat  --");
    lv_obj_set_style_text_font(s_home_lat_lbl, &lv_font_montserrat_12, 0);

    s_home_lng_lbl = lv_label_create(gps_box);
    lv_label_set_text(s_home_lng_lbl, "Long --");
    lv_obj_set_style_text_font(s_home_lng_lbl, &lv_font_montserrat_12, 0);

    return bar;
}

void ui_home_topbar_update_datetime(const char *date, const char *time)
{
    if (s_home_date_lbl && date) lv_label_set_text(s_home_date_lbl, date);
    if (s_home_time_lbl && time) lv_label_set_text(s_home_time_lbl, time);
}

void ui_home_topbar_update_gps(const char *lat, const char *lng)
{
    if (s_home_lat_lbl && lat) lv_label_set_text(s_home_lat_lbl, lat);
    if (s_home_lng_lbl && lng) lv_label_set_text(s_home_lng_lbl, lng);
}

void ui_home_topbar_update_sd(bool inserted)
{
    /* HOME topbar no longer displays SD-card status.
       Keep this API for compatibility with existing controller code. */
    LV_UNUSED(inserted);
}

/* ------------------------------------------------------------------------ */
/*  Generic sub-page top bar (Sampling/Trend/Data/Calibration/System)       */
/* ------------------------------------------------------------------------ */
static void back_btn_event_cb(lv_event_t * e)
{
    ui_page_id_t back_page = (ui_page_id_t)(uintptr_t)lv_event_get_user_data(e);
    ui_navigate_to(back_page);
}

lv_obj_t * ui_create_subpage_topbar(lv_obj_t * parent, const char * title, ui_page_id_t back_page)
{
    lv_obj_t * bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, UI_HOR_RES, UI_SUBPAGE_TOPBAR_H);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, UI_COLOR_NAVY_DARK, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(bar, 16, 0);

    /* Left group: back arrow + title */
    lv_obj_t * left = lv_obj_create(bar);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left, 12, 0);

    lv_obj_t * back_btn = lv_obj_create(left);
    lv_obj_remove_style_all(back_btn);
    lv_obj_set_size(back_btn, 32, 32);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)back_page);
    lv_obj_t * back_icon = lv_label_create(back_btn);
    lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_icon, UI_COLOR_WHITE, 0);
    lv_obj_set_style_text_font(back_icon, &lv_font_montserrat_20, 0);
    lv_obj_center(back_icon);

    lv_obj_t * title_lbl = lv_label_create(left);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, UI_COLOR_WHITE, 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_18, 0);

    /* Right group: clock + sd + wifi */
    lv_obj_t * right = lv_obj_create(bar);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 14, 0);

    lv_obj_t * clock_lbl = lv_label_create(right);
    lv_label_set_text(clock_lbl, "10:30");
    lv_obj_set_style_text_color(clock_lbl, UI_COLOR_WHITE, 0);
    lv_obj_set_style_text_font(clock_lbl, &lv_font_montserrat_14, 0);

    lv_obj_t * sd_icon = lv_label_create(right);
    lv_label_set_text(sd_icon, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(sd_icon, UI_COLOR_WHITE, 0);
    lv_obj_set_style_text_font(sd_icon, &lv_font_montserrat_18, 0);

    lv_obj_t * wifi_icon = lv_label_create(right);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_icon, UI_COLOR_WHITE, 0);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_18, 0);

    return bar;
}

/* ------------------------------------------------------------------------ */
/*  Card container                                                          */
/* ------------------------------------------------------------------------ */
lv_obj_t * ui_create_card(lv_obj_t * parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t * card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/* ------------------------------------------------------------------------ */
/*  Stat tile (TEMP / HUMIDITY / PRESSURE / WIND ...)                       */
/* ------------------------------------------------------------------------ */
lv_obj_t * ui_create_stat_tile(lv_obj_t * parent, const char * symbol,
                                const char * value, const char * unit, const char * caption)
{
    lv_obj_t * tile = lv_obj_create(parent);
    lv_obj_remove_style_all(tile);
    lv_obj_set_size(tile, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(tile, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tile, 8, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * icon = lv_label_create(tile);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_BLUE, 0);

    lv_obj_t * text_col = lv_obj_create(tile);
    lv_obj_remove_style_all(text_col);
    lv_obj_set_size(text_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * value_row = lv_obj_create(text_col);
    lv_obj_remove_style_all(value_row);
    lv_obj_set_size(value_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(value_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(value_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(value_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(value_row, 4, 0);

    lv_obj_t * value_lbl = lv_label_create(value_row);
    lv_label_set_text(value_lbl, value);
    lv_obj_set_style_text_font(value_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(value_lbl, UI_COLOR_TEXT_DARK, 0);

    lv_obj_t * unit_lbl = lv_label_create(value_row);
    lv_label_set_text(unit_lbl, unit);
    lv_obj_set_style_text_font(unit_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(unit_lbl, UI_COLOR_TEXT_MUTED, 0);

    lv_obj_t * caption_lbl = lv_label_create(text_col);
    lv_label_set_text(caption_lbl, caption);
    lv_obj_set_style_text_font(caption_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(caption_lbl, UI_COLOR_TEXT_MUTED, 0);

    return value_lbl; /* caller keeps this pointer to update the reading later */
}

/* ------------------------------------------------------------------------ */
/*  Settings row (Label ..... Value >)                                      */
/* ------------------------------------------------------------------------ */
lv_obj_t * ui_create_settings_row(lv_obj_t * parent, const char * label, const char * value)
{
    lv_obj_t * row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), 48);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(row, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label_lbl = lv_label_create(row);
    lv_label_set_text(label_lbl, label);
    lv_obj_set_style_text_font(label_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_lbl, UI_COLOR_TEXT_DARK, 0);

    lv_obj_t * right_group = lv_obj_create(row);
    lv_obj_remove_style_all(right_group);
    lv_obj_set_size(right_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(right_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right_group, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right_group, 8, 0);

    lv_obj_t * value_lbl = lv_label_create(right_group);
    lv_label_set_text(value_lbl, value);
    lv_obj_set_style_text_font(value_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(value_lbl, UI_COLOR_TEXT_MUTED, 0);

    lv_obj_t * chevron = lv_label_create(right_group);
    lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(chevron, UI_COLOR_TEXT_MUTED, 0);

    return value_lbl; /* caller can update this if the value changes at runtime */
}

lv_obj_t * ui_settings_row_get_row(lv_obj_t * value_label)
{
    if (!value_label) return NULL;

    lv_obj_t * right_group = lv_obj_get_parent(value_label);
    if (!right_group) return NULL;

    return lv_obj_get_parent(right_group);
}

/* ------------------------------------------------------------------------ */
/*  Action button (START / PAUSE / STOP)                                    */
/* ------------------------------------------------------------------------ */
lv_obj_t * ui_create_action_button(lv_obj_t * parent, const char * symbol, const char * text,
                                    lv_color_t bg_color, lv_color_t bg_color_pressed)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_style_bg_color(btn, bg_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, bg_color_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, 56);

    lv_obj_t * row = lv_obj_create(btn);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(row);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * icon = lv_label_create(row);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_color(icon, UI_COLOR_WHITE, 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, 0);

    lv_obj_t * lbl = lv_label_create(row);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, UI_COLOR_WHITE, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);

    return btn;
}