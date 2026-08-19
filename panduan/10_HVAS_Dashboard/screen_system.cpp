/**
 * SYSTEM page - HVAS TSP
 *
 * Matches the System mock-up:
 *   Date & Time
 *   Device ID
 *   Firmware
 *   About
 *   REBOOT / SHUTDOWN
 *
 * Notes:
 * - Date & Time is functional through the ATmega RTC command: set_rtc.
 * - Device ID / Firmware are informational values supplied by get_system.
 * - Shutdown uses a confirmation dialog and reports that true power-off is not
 *   available from software on this hardware. Reboot is functional.
 */

#include "ui_common.h"
#include "ui_screens.h"
#include "communication.h"

#include <Arduino.h>
#include <esp_system.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Persistent screen widgets                                                   */
/* -------------------------------------------------------------------------- */
static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_row_datetime = NULL;
static lv_obj_t *s_row_device = NULL;
static lv_obj_t *s_row_firmware = NULL;

static lv_obj_t *s_val_datetime = NULL;
static lv_obj_t *s_val_device = NULL;
static lv_obj_t *s_val_firmware = NULL;

static lv_obj_t *s_popup = NULL;
static lv_obj_t *s_popup_keyboard = NULL;
static lv_obj_t *s_popup_input1 = NULL;
static lv_obj_t *s_popup_input2 = NULL;
static lv_obj_t *s_popup_info = NULL;

static uint32_t s_last_status_refresh = 0;

/* -------------------------------------------------------------------------- */
/* Small helpers                                                               */
/* -------------------------------------------------------------------------- */
static void popup_close(void)
{
    if (s_popup_keyboard)
    {
        lv_obj_del(s_popup_keyboard);
        s_popup_keyboard = NULL;
    }

    if (s_popup)
    {
        lv_obj_del(s_popup);
        s_popup = NULL;
    }

    s_popup_input1 = NULL;
    s_popup_input2 = NULL;
    s_popup_info = NULL;
}

static lv_obj_t *popup_create(const char *title, lv_coord_t w, lv_coord_t h)
{
    popup_close();

    s_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_popup, w, h);
    lv_obj_center(s_popup);
    lv_obj_set_style_radius(s_popup, 16, 0);
    lv_obj_set_style_bg_color(s_popup, UI_COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(s_popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_popup, 2, 0);
    lv_obj_set_style_border_color(s_popup, UI_COLOR_BLUE, 0);
    lv_obj_set_style_shadow_width(s_popup, 20, 0);
    lv_obj_clear_flag(s_popup, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(s_popup);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_DARK, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 14);

    return s_popup;
}

static lv_obj_t *popup_button(const char *text, lv_coord_t x, lv_coord_t y,
                              lv_coord_t w, lv_color_t color, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(s_popup);
    lv_obj_set_size(btn, w, 48);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, UI_COLOR_WHITE, 0);
    lv_obj_center(lbl);

    return btn;
}

static void popup_ok_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    popup_close();
}

static void popup_cancel_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    popup_close();
}

static void datetime_apply_cb(lv_event_t *e);

/* Keyboard READY/CANCEL must be handled explicitly.  Otherwise the
 * keyboard checkmark only changes the keyboard state and does not apply the
 * value in our popup. */
static void datetime_keyboard_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY)
        datetime_apply_cb(e);
    else if (code == LV_EVENT_CANCEL)
        popup_close();
}

static void show_info_popup(const char *title, const char *body)
{
    popup_create(title, 560, 250);

    s_popup_info = lv_label_create(s_popup);
    lv_label_set_text(s_popup_info, body);
    lv_obj_set_width(s_popup_info, 500);
    lv_label_set_long_mode(s_popup_info, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(s_popup_info, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_popup_info, UI_COLOR_TEXT_DARK, 0);
    lv_obj_align(s_popup_info, LV_ALIGN_TOP_MID, 0, 60);

    popup_button("OK", 180, 185, 200, UI_COLOR_BLUE, popup_ok_cb);
}

/* -------------------------------------------------------------------------- */
/* Date & time                                                                 */
/* -------------------------------------------------------------------------- */
static bool validate_datetime_digits(const char *s)
{
    if (!s || strlen(s) != 14)
        return false;

    for (int i = 0; i < 14; ++i)
    {
        if (s[i] < '0' || s[i] > '9')
            return false;
    }

    int year = atoi(String(s).substring(0, 4).c_str());
    int month = atoi(String(s).substring(4, 6).c_str());
    int day = atoi(String(s).substring(6, 8).c_str());
    int hour = atoi(String(s).substring(8, 10).c_str());
    int minute = atoi(String(s).substring(10, 12).c_str());
    int second = atoi(String(s).substring(12, 14).c_str());

    return year >= 2024 && month >= 1 && month <= 12 &&
           day >= 1 && day <= 31 && hour >= 0 && hour <= 23 &&
           minute >= 0 && minute <= 59 && second >= 0 && second <= 59;
}

static void datetime_apply_cb(lv_event_t *e)
{
    LV_UNUSED(e);

    if (!s_popup_input1)
        return;

    const char *digits = lv_textarea_get_text(s_popup_input1);

    if (!validate_datetime_digits(digits))
    {
        if (s_popup_info)
            lv_label_set_text(s_popup_info,
                              "Invalid format. Enter 14 digits:\nYYYYMMDDHHMMSS");
        return;
    }

    String d(digits);
    String iso = d.substring(0, 4) + "-" +
                 d.substring(4, 6) + "-" +
                 d.substring(6, 8) + " " +
                 d.substring(8, 10) + ":" +
                 d.substring(10, 12) + ":" +
                 d.substring(12, 14);

    String cmd = String("{\"cmd\":\"set_rtc\",\"datetime\":\"") +
                 iso + "\"}";

    Serial.print("[SYSTEM] SET RTC -> ");
    Serial.println(iso);

    String response = sendCommand(cmd);
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);

    if (err || doc["status"] != "ok")
    {
        if (s_popup_info)
            lv_label_set_text(
                s_popup_info,
                "RTC update failed. Check RS485/ATmega."
            );

        return;
    }

    /*
    * Immediately read RTC again so systemInfo.datetime
    * and the LCD are updated without waiting for the
    * next communication cycle.
    */
    comm_refresh_rtc();

    popup_close();
}

static void show_datetime_popup(void)
{
    /* Keep the dialog and numeric keyboard inside the 800x480 viewport.
     * The keyboard is deliberately a CHILD of the popup (not a separate
     * top-layer object).  This matches the working Home-page input popup and
     * prevents the keyboard from disappearing/being clipped by the overlay. */
    popup_create("DATE & TIME", 700, 460);
    lv_obj_set_pos(s_popup, 50, 10);

    lv_obj_t *hint = lv_label_create(s_popup);
    lv_label_set_text(hint, "Enter 14 digits: YYYYMMDDHHMMSS");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(hint, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 42);

    s_popup_info = lv_label_create(s_popup);
    lv_label_set_text(s_popup_info, "Example: 20260811123000");
    lv_obj_set_style_text_font(s_popup_info, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_popup_info, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(s_popup_info, LV_ALIGN_TOP_MID, 0, 68);

    s_popup_input1 = lv_textarea_create(s_popup);
    lv_obj_set_size(s_popup_input1, 560, 44);
    lv_obj_align(s_popup_input1, LV_ALIGN_TOP_MID, 0, 94);
    lv_textarea_set_one_line(s_popup_input1, true);
    lv_textarea_set_max_length(s_popup_input1, 14);
    lv_textarea_set_placeholder_text(s_popup_input1, "YYYYMMDDHHMMSS");
    lv_obj_set_style_text_font(s_popup_input1, &lv_font_montserrat_18, 0);

    popup_button("SET", 125, 150, 180, UI_COLOR_GREEN, datetime_apply_cb);
    popup_button("CANCEL", 395, 150, 180, UI_COLOR_RED, popup_cancel_cb);

    /* Numeric keyboard: child of popup, bottom area. */
    s_popup_keyboard = lv_keyboard_create(s_popup);
    lv_obj_set_size(s_popup_keyboard, 660, 180);
    lv_obj_align(s_popup_keyboard, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_keyboard_set_mode(s_popup_keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(s_popup_keyboard, s_popup_input1);
    lv_obj_add_event_cb(s_popup_keyboard, datetime_keyboard_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(s_popup_keyboard, datetime_keyboard_cb, LV_EVENT_CANCEL, NULL);
}

/* -------------------------------------------------------------------------- */
/* Reboot / shutdown                                                          */
/* -------------------------------------------------------------------------- */
static void reboot_confirm_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    popup_close();
    delay(50);
    esp_restart();
}

static void show_reboot_confirm(void)
{
    popup_create("REBOOT", 520, 230);

    s_popup_info = lv_label_create(s_popup);
    lv_label_set_text(s_popup_info, "Restart the HVAS controller now?");
    lv_obj_set_style_text_font(s_popup_info, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_popup_info, UI_COLOR_TEXT_DARK, 0);
    lv_obj_align(s_popup_info, LV_ALIGN_TOP_MID, 0, 65);

    popup_button("REBOOT", 75, 145, 160, UI_COLOR_BLUE, reboot_confirm_cb);
    popup_button("CANCEL", 285, 145, 160, UI_COLOR_RED, popup_cancel_cb);
}

static void show_shutdown_info(void)
{
    show_info_popup(
        "SHUTDOWN",
        "True power-off is not available through software\non this hardware configuration.\n\nUse the physical power switch to turn the unit off.");
}

/* -------------------------------------------------------------------------- */
/* Row callbacks                                                              */
/* -------------------------------------------------------------------------- */
static void sys_row_cb(lv_event_t *e)
{
    const char *which = (const char *)lv_event_get_user_data(e);
    if (!which)
        return;

    if (strcmp(which, "datetime") == 0)
        show_datetime_popup();
    else if (strcmp(which, "device_id") == 0)
    {
        char buf[160];
        snprintf(buf, sizeof(buf), "Device ID:\n%s\n\nThis value is supplied by the ATmega system module.",
                 systemInfo.device.length() ? systemInfo.device.c_str() : "N/A");
        show_info_popup("DEVICE ID", buf);
    }
    else if (strcmp(which, "firmware") == 0)
    {
        char buf[160];
        snprintf(buf, sizeof(buf), "Firmware:\n%s\n\nFirmware information is read from get_system.",
                 systemInfo.version.length() ? systemInfo.version.c_str() : "N/A");
        show_info_popup("FIRMWARE", buf);
    }
    else if (strcmp(which, "about") == 0)
    {
        show_info_popup(
            "ABOUT",
            "ENVILIFE\nHVAS TSP - High Volume Air Sampler\n\nESP32-S3 Touch LCD 7\nLVGL 8.4\nRS485 + ATmega co-processor");
    }
}

static void reboot_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    show_reboot_confirm();
}

static void shutdown_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    show_shutdown_info();
}

/* -------------------------------------------------------------------------- */
/* Live status refresh                                                        */
/* -------------------------------------------------------------------------- */
static void refresh_system_status(void)
{
    if (!s_screen)
        return;

    if (millis() - s_last_status_refresh < 2000)
        return;

    s_last_status_refresh = millis();

    /* Device / firmware are populated by screen_system_update(). */
}

/* Compact the four visible settings rows so the complete System page fits inside the
 * 800x480 viewport together with the reboot/shutdown buttons. */
static void compact_system_row(lv_obj_t *row)
{
    if (!row) return;

    /* Keep every row inside the card.  In particular, long values such as
     * the ATmega Device ID must never push the right side outside the LCD. */
    lv_obj_set_height(row, 42);
    lv_obj_set_style_min_height(row, 42, 0);
    lv_obj_set_style_max_height(row, 42, 0);
    lv_obj_set_style_pad_hor(row, 8, 0);
    lv_obj_set_flex_grow(row, 0);

    /* row children: label + right_group(value + chevron) */
    lv_obj_t *label = lv_obj_get_child(row, 0);
    lv_obj_t *right = lv_obj_get_child(row, 1);

    if (label)
    {
        lv_obj_set_width(label, 210);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    }

    if (right)
    {
        /* Fixed width keeps the right side inside the card. */
        lv_obj_set_width(right, 360);
        lv_obj_set_height(right, LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(right, 0);
        lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(right, 6, 0);

        lv_obj_t *value = lv_obj_get_child(right, 0);
        if (value)
        {
            lv_obj_set_width(value, 325);
            lv_label_set_long_mode(value, LV_LABEL_LONG_DOT);
            lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_set_style_text_font(value, &lv_font_montserrat_18, 0);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */
lv_obj_t *screen_system_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    s_screen = scr;

    lv_obj_set_style_bg_color(scr, UI_COLOR_PAGE_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_subpage_topbar(scr, "SYSTEM", UI_PAGE_HOME);

    /* 800x480 display: reserve a small bottom area for REBOOT/SHUTDOWN. */
    lv_obj_t *card = ui_create_card(
        scr,
        UI_HOR_RES - 32,
        UI_VER_RES - UI_SUBPAGE_TOPBAR_H - 16);

    lv_obj_set_pos(card, 16, UI_SUBPAGE_TOPBAR_H + 8);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *r1 = ui_create_settings_row(card, "Date & Time", "N/A");
    s_row_datetime = ui_settings_row_get_row(r1);
    s_val_datetime = r1;
    lv_obj_add_event_cb(s_row_datetime, sys_row_cb, LV_EVENT_CLICKED, (void *)"datetime");

    lv_obj_t *r5 = ui_create_settings_row(card, "Device ID", "N/A");
    s_row_device = ui_settings_row_get_row(r5);
    s_val_device = r5;
    lv_obj_add_event_cb(s_row_device, sys_row_cb, LV_EVENT_CLICKED, (void *)"device_id");

    lv_obj_t *r6 = ui_create_settings_row(card, "Firmware", "N/A");
    s_row_firmware = ui_settings_row_get_row(r6);
    s_val_firmware = r6;
    lv_obj_add_event_cb(s_row_firmware, sys_row_cb, LV_EVENT_CLICKED, (void *)"firmware");

    lv_obj_t *r7 = ui_create_settings_row(card, "About", "");
    lv_obj_t *row7 = ui_settings_row_get_row(r7);
    lv_obj_add_event_cb(row7, sys_row_cb, LV_EVENT_CLICKED, (void *)"about");
    lv_obj_set_style_border_width(row7, 0, 0);

    compact_system_row(s_row_datetime);
    compact_system_row(s_row_device);
    compact_system_row(s_row_firmware);
    compact_system_row(row7);

    /* No flex-growing spacer here: it used to push the action buttons below
     * the visible area on some LVGL/font configurations. */
    lv_obj_t *spacer = lv_obj_create(card);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_flex_grow(spacer, 0);
    lv_obj_set_size(spacer, 1, 4);

    lv_obj_t *btn_row = lv_obj_create(card);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_size(btn_row, lv_pct(100), 48);
    lv_obj_set_flex_grow(btn_row, 0);
    lv_obj_set_layout(btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btn_row, 10, 0);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *reboot = lv_btn_create(btn_row);
    lv_obj_set_style_bg_opa(reboot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(reboot, UI_COLOR_BLUE, 0);
    lv_obj_set_style_border_width(reboot, 1, 0);
    lv_obj_set_style_shadow_width(reboot, 0, 0);
    lv_obj_set_style_radius(reboot, 8, 0);
    lv_obj_set_flex_grow(reboot, 1);
    lv_obj_set_height(reboot, 46);
    lv_obj_add_event_cb(reboot, reboot_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *rl = lv_label_create(reboot);
    lv_label_set_text(rl, LV_SYMBOL_REFRESH " REBOOT");
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(rl, UI_COLOR_BLUE, 0);
    lv_obj_center(rl);

    lv_obj_t *shutdown = lv_btn_create(btn_row);
    lv_obj_set_style_bg_opa(shutdown, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(shutdown, UI_COLOR_RED, 0);
    lv_obj_set_style_border_width(shutdown, 1, 0);
    lv_obj_set_style_shadow_width(shutdown, 0, 0);
    lv_obj_set_style_radius(shutdown, 8, 0);
    lv_obj_set_flex_grow(shutdown, 1);
    lv_obj_set_height(shutdown, 46);
    lv_obj_add_event_cb(shutdown, shutdown_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *sl = lv_label_create(shutdown);
    lv_label_set_text(sl, LV_SYMBOL_POWER " SHUTDOWN");
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(sl, UI_COLOR_RED, 0);
    lv_obj_center(sl);

    refresh_system_status();
    return scr;
}

void screen_system_update(const char *datetime, const char *device, const char *version)
{
    if (!s_screen)
        return;

    if (s_val_datetime)
    {
        if (datetime && strlen(datetime) >= 19)
        {
            char buf[32];
            int year = 0, month = 0, day = 0;
            int hour = 0, minute = 0;
            sscanf(datetime, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute);
            snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d",
                     day, month, year, hour, minute);
            lv_label_set_text(s_val_datetime, buf);
        }
        else
            lv_label_set_text(s_val_datetime, "N/A");
    }

    if (s_val_device)
        lv_label_set_text(s_val_device, (device && strlen(device)) ? device : "N/A");

    if (s_val_firmware)
        lv_label_set_text(s_val_firmware, (version && strlen(version)) ? version : "N/A");

    refresh_system_status();
}

#ifdef __cplusplus
}
#endif
