/**
 * ui_common.h
 * HVAS TSP 2026 - envilife High Volume Air Sampler Dashboard
 * Shared colors, dimensions, and helper-widget builders used by every screen.
 *
 * Target: Waveshare ESP32-S3-Touch-LCD-7 (800 x 480), LVGL v8.3
 */
#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */
/*  Screen geometry                                                        */
/* ---------------------------------------------------------------------- */
#define UI_HOR_RES          800
#define UI_VER_RES          480

#define UI_SIDEBAR_WIDTH    140
#define UI_TOPBAR_HEIGHT    70
#define UI_SUBPAGE_TOPBAR_H 56

/* ---------------------------------------------------------------------- */
/*  Palette (matches the envilife GUI mock-up)                            */
/* ---------------------------------------------------------------------- */
#define UI_COLOR_NAVY_DARK      lv_color_hex(0x0E1621)   /* sidebar / topbar bg  */
#define UI_COLOR_NAVY_DARKER    lv_color_hex(0x0A121B)
#define UI_COLOR_TEAL_ACTIVE    lv_color_hex(0x0F8C8C)   /* active sidebar item  */
#define UI_COLOR_PAGE_BG        lv_color_hex(0xF3F4F6)   /* light grey page bg   */
#define UI_COLOR_CARD_BG        lv_color_hex(0xFFFFFF)
#define UI_COLOR_CARD_BORDER    lv_color_hex(0xE5E7EB)
#define UI_COLOR_TEXT_DARK      lv_color_hex(0x1F2937)
#define UI_COLOR_TEXT_MUTED     lv_color_hex(0x6B7280)
#define UI_COLOR_GREEN          lv_color_hex(0x16A34A)   /* OK / RUNNING / START */
#define UI_COLOR_GREEN_DARK     lv_color_hex(0x15803D)
#define UI_COLOR_BLUE           lv_color_hex(0x2563EB)   /* links / PAUSE        */
#define UI_COLOR_BLUE_DARK      lv_color_hex(0x1D4ED8)
#define UI_COLOR_RED            lv_color_hex(0xDC2626)   /* STOP / warnings      */
#define UI_COLOR_RED_DARK       lv_color_hex(0xB91C1C)
#define UI_COLOR_WHITE          lv_color_hex(0xFFFFFF)

/* ---------------------------------------------------------------------- */
/*  Page identifiers used for sidebar navigation                          */
/* ---------------------------------------------------------------------- */
typedef enum {
    UI_PAGE_HOME = 0,
    UI_PAGE_SAMPLING,
    UI_PAGE_TREND,
    UI_PAGE_DATA,
    UI_PAGE_SYSTEM,
    UI_PAGE_CALIBRATION,   /* reached from System, not from the sidebar */
    UI_PAGE_COUNT
} ui_page_id_t;

/* Call this any time to switch the visible screen (handles create-on-first-use) */
void ui_navigate_to(ui_page_id_t page);

/* ---------------------------------------------------------------------- */
/*  Helper widgets shared by several screens                              */
/* ---------------------------------------------------------------------- */

/* A left-hand icon+label navigation column, used only on the Home screen. */
lv_obj_t * ui_create_sidebar(lv_obj_t * parent, ui_page_id_t active_page);

/* Home screen's top status bar: logo, title, date/time, GPS, SD status. */
lv_obj_t * ui_create_home_topbar(lv_obj_t * parent);

void ui_home_topbar_update_datetime(
    const char *date,
    const char *time);

void ui_home_topbar_update_gps(
    const char *lat,
    const char *lng);

void ui_home_topbar_update_sd(
    bool inserted);

/* Returns the actual clickable row that owns a value label returned by
 * ui_create_settings_row(). */
lv_obj_t * ui_settings_row_get_row(lv_obj_t * value_label);

/* Generic sub-page top bar: back arrow + title + clock + SD/WiFi icons.
 * back_page is where the back arrow should navigate to (usually UI_PAGE_HOME). */
lv_obj_t * ui_create_subpage_topbar(lv_obj_t * parent, const char * title, ui_page_id_t back_page);

/* A white rounded "card" container with a title row, used across all screens. */
lv_obj_t * ui_create_card(lv_obj_t * parent, lv_coord_t w, lv_coord_t h);

/* A small stat tile like the TEMP / HUMIDITY / PRESSURE / WIND boxes on Home.
 * Returns the value label so callers can update it later. */
lv_obj_t * ui_create_stat_tile(lv_obj_t * parent, const char * symbol,
                                const char * value, const char * unit, const char * caption);

/* A settings-style row: "Label ............ Value >" used on Sampling/System/Calibration. */
lv_obj_t * ui_create_settings_row(lv_obj_t * parent, const char * label, const char * value);

/* A colored action button with icon + text (START/PAUSE/STOP style). */
lv_obj_t * ui_create_action_button(lv_obj_t * parent, const char * symbol, const char * text,
                                    lv_color_t bg_color, lv_color_t bg_color_pressed);

#ifdef __cplusplus
}
#endif

#endif /* UI_COMMON_H */