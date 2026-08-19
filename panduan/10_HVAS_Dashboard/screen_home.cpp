/**
 * screen_home.c
 * HOME page: flow rate, total volume, run status, environmental sensors,
 * device indicators (filter/SD/printer/power), and START/PAUSE/STOP controls.
 */
#include "ui_common.h"
#include "ui_screens.h"
#include "home_controller.h"
#include "screen_home.h"
#include "result_controller.h"
#include <stdio.h>

/* Card semantics: blue-tinted cards are user-configurable; white cards are read-only. */
static const lv_color_t UI_COLOR_EDITABLE_CARD = lv_color_hex(0xDCEBFA);
static const lv_color_t UI_COLOR_EDITABLE_BORDER = lv_color_hex(0x93C5FD);
#include <string.h>
#include <stdlib.h>

/* Keep pointers to the labels/widgets we need to update at runtime, so the
 * caller's sensor-polling task can push live values without rebuilding UI. */
static lv_obj_t * s_lbl_flow_value;
static lv_obj_t * s_lbl_flow_status_dot;
static lv_obj_t * s_bar_flow;

static uint32_t s_flow_setpoint = 2000;

static lv_obj_t * s_lbl_flow_setpoint;
static lv_obj_t * s_lbl_total_volume;
static lv_obj_t * s_lbl_instant_flow;
static lv_obj_t * s_lbl_run_status;
static lv_obj_t * s_lbl_sampling_timer;
static lv_obj_t * s_lbl_remaining_time;
static lv_obj_t * s_lbl_power;

static lv_obj_t * s_lbl_filter;
static lv_obj_t * s_lbl_sd;
static lv_obj_t * s_lbl_printer;

static lv_obj_t * s_btn_start;
static lv_obj_t * s_btn_pause;
static lv_obj_t * s_btn_stop;

/* Timer setting popup */
static lv_obj_t * s_timer_popup = NULL;
static lv_obj_t * s_timer_textarea = NULL;
static lv_obj_t * s_timer_keyboard = NULL;

/* Flow-rate setting popup */
static lv_obj_t * s_flow_popup = NULL;
static lv_obj_t * s_flow_textarea = NULL;
static lv_obj_t * s_flow_keyboard = NULL;
static lv_obj_t * s_result_popup = NULL;

static void show_result_popup();
static lv_obj_t * s_val_temp;
static lv_obj_t * s_val_humidity;
static lv_obj_t * s_val_pressure;
static lv_obj_t * s_val_wind_speed;
static lv_obj_t * s_val_wind_dir;

static void flow_card_cb(lv_event_t * e);
static void show_flow_popup();
static void flow_popup_close();
static void flow_popup_close_cb(lv_event_t * e);
static void flow_set_cb(lv_event_t * e);

static void flow_popup_close()
{
    if (s_flow_popup)
    {
        lv_obj_del(s_flow_popup);

        s_flow_popup = NULL;
        s_flow_textarea = NULL;
        s_flow_keyboard = NULL;
    }
}

static void flow_popup_close_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    flow_popup_close();
}

static void flow_card_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    show_flow_popup();
}

static void show_flow_popup()
{
    if (s_flow_popup)
        return;

    s_flow_popup = lv_obj_create(lv_layer_top());

    lv_obj_set_size(
        s_flow_popup,
        520,
        450
    );

    lv_obj_center(s_flow_popup);

    lv_obj_set_style_radius(
        s_flow_popup,
        16,
        0
    );

    lv_obj_set_style_bg_color(
        s_flow_popup,
        lv_color_white(),
        0
    );

    lv_obj_set_style_bg_opa(
        s_flow_popup,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        s_flow_popup,
        2,
        0
    );

    lv_obj_set_style_border_color(
        s_flow_popup,
        UI_COLOR_TEXT_MUTED,
        0
    );

    lv_obj_clear_flag(
        s_flow_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );


    /* Title */
    lv_obj_t * title =
        lv_label_create(s_flow_popup);

    lv_label_set_text(
        title,
        "FLOW RATE SETTINGS"
    );

    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_set_style_text_color(
        title,
        UI_COLOR_TEXT_DARK,
        0
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        15
    );


    /* Description */
    lv_obj_t * description =
        lv_label_create(s_flow_popup);

    lv_label_set_text(
        description,
        "Set flow rate setpoint (LPM)"
    );

    lv_obj_set_style_text_font(
        description,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_style_text_color(
        description,
        UI_COLOR_TEXT_MUTED,
        0
    );

    lv_obj_align(
        description,
        LV_ALIGN_TOP_MID,
        0,
        50
    );


    /* Current setpoint */
    char flow_text[16];

    snprintf(
        flow_text,
        sizeof(flow_text),
        "%lu",
        (unsigned long)s_flow_setpoint
    );


    /* Textarea */
    s_flow_textarea =
        lv_textarea_create(s_flow_popup);

    lv_obj_set_size(
        s_flow_textarea,
        220,
        50
    );

    lv_obj_align(
        s_flow_textarea,
        LV_ALIGN_TOP_MID,
        0,
        80
    );

    lv_textarea_set_one_line(
        s_flow_textarea,
        true
    );

    lv_textarea_set_accepted_chars(
        s_flow_textarea,
        "0123456789"
    );

    lv_textarea_set_text(
        s_flow_textarea,
        flow_text
    );

    lv_textarea_set_cursor_pos(
        s_flow_textarea,
        LV_TEXTAREA_CURSOR_LAST
    );

    lv_obj_set_style_text_font(
        s_flow_textarea,
        &lv_font_montserrat_20,
        0
    );


    /* SET */
    lv_obj_t * set_btn =
        lv_btn_create(s_flow_popup);

    lv_obj_set_size(
        set_btn,
        120,
        45
    );

    lv_obj_align(
        set_btn,
        LV_ALIGN_TOP_MID,
        -70,
        145
    );

    lv_obj_add_event_cb(
        set_btn,
        flow_set_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t * set_label =
        lv_label_create(set_btn);

    lv_label_set_text(
        set_label,
        "SET"
    );

    lv_obj_center(set_label);


    /* CLOSE */
    lv_obj_t * close_btn =
        lv_btn_create(s_flow_popup);

    lv_obj_set_size(
        close_btn,
        120,
        45
    );

    lv_obj_align(
        close_btn,
        LV_ALIGN_TOP_MID,
        70,
        145
    );

    lv_obj_add_event_cb(
        close_btn,
        flow_popup_close_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t * close_label =
        lv_label_create(close_btn);

    lv_label_set_text(
        close_label,
        "CLOSE"
    );

    lv_obj_center(close_label);


    /* Numeric keyboard */
    s_flow_keyboard =
        lv_keyboard_create(s_flow_popup);

    lv_obj_set_size(
        s_flow_keyboard,
        460,
        180
    );

    lv_obj_align(
        s_flow_keyboard,
        LV_ALIGN_BOTTOM_MID,
        0,
        -10
    );

    lv_keyboard_set_mode(
        s_flow_keyboard,
        LV_KEYBOARD_MODE_NUMBER
    );

    lv_keyboard_set_textarea(
        s_flow_keyboard,
        s_flow_textarea
    );
}

static void flow_set_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    if (!s_flow_textarea)
        return;

    const char * text =
        lv_textarea_get_text(s_flow_textarea);

    if (!text || text[0] == '\0')
        return;

    unsigned long value =
        strtoul(text, NULL, 10);

    if (value == 0)
        return;

    /*
     * Temporary safe range.
     * We will confirm the real hardware range
     * before sending commands to ATmega.
     */
    if (value > 2000)
        return;

    s_flow_setpoint =
        (uint32_t)value;

    if (s_lbl_flow_setpoint)
    {
        char buf[24];

        snprintf(
            buf,
            sizeof(buf),
            "%lu LPM",
            (unsigned long)s_flow_setpoint
        );

        lv_label_set_text(
            s_lbl_flow_setpoint,
            buf
        );
    }

    flow_popup_close();
}

static void result_popup_close()
{
    if (s_result_popup)
    {
        lv_obj_del(s_result_popup);
        s_result_popup = NULL;
    }
}

static void result_print_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    Serial.println("[LCD] PRINT BUTTON CLICKED");

    bool ok = result_print();

    Serial.print("[LCD] PRINT RESULT = ");
    Serial.println(ok ? "SUCCESS" : "FAILED");

    result_popup_close();
}

static void result_save_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    result_popup_close();

    result_save();
}

static void result_print_save_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    result_popup_close();

    result_print_and_save();
}

static void show_result_popup()
{
    if (s_result_popup)
        return;

    s_result_popup = lv_obj_create(lv_layer_top());

    lv_obj_set_size(
        s_result_popup,
        500,
        260
    );

    lv_obj_center(s_result_popup);

    lv_obj_set_style_radius(
        s_result_popup,
        16,
        0
    );

    lv_obj_set_style_bg_color(
        s_result_popup,
        lv_color_white(),
        0
    );

    lv_obj_set_style_bg_opa(
        s_result_popup,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        s_result_popup,
        2,
        0
    );

    lv_obj_set_style_border_color(
        s_result_popup,
        UI_COLOR_TEXT_MUTED,
        0
    );

    lv_obj_clear_flag(
        s_result_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );


    /* ---------------------------------------------------- */
    /* Title                                                */
    /* ---------------------------------------------------- */

    lv_obj_t * title =
        lv_label_create(s_result_popup);

    lv_label_set_text(
        title,
        "SAMPLING SELESAI"
    );

    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_set_style_text_color(
        title,
        UI_COLOR_TEXT_DARK,
        0
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        20
    );


    /* ---------------------------------------------------- */
    /* Description                                          */
    /* ---------------------------------------------------- */

    lv_obj_t * desc =
        lv_label_create(s_result_popup);

    lv_label_set_text(
        desc,
        "Pilih tindakan untuk hasil sampling:"
    );

    lv_obj_set_style_text_font(
        desc,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_style_text_color(
        desc,
        UI_COLOR_TEXT_MUTED,
        0
    );

    lv_obj_align(
        desc,
        LV_ALIGN_TOP_MID,
        0,
        55
    );


    /* ---------------------------------------------------- */
    /* PRINT & SAVE                                         */
    /* ---------------------------------------------------- */

    lv_obj_t * btn_print_save =
        ui_create_action_button(
            s_result_popup,
            LV_SYMBOL_SAVE,
            "PRINT & SAVE",
            UI_COLOR_GREEN,
            UI_COLOR_GREEN_DARK
        );

    lv_obj_set_width(
        btn_print_save,
        200
    );

    lv_obj_align(
        btn_print_save,
        LV_ALIGN_TOP_MID,
        0,
        95
    );

    lv_obj_add_event_cb(
        btn_print_save,
        result_print_save_cb,
        LV_EVENT_CLICKED,
        NULL
    );


    /* ---------------------------------------------------- */
    /* PRINT ONLY                                           */
    /* ---------------------------------------------------- */

    lv_obj_t * btn_print =
        ui_create_action_button(
            s_result_popup,
            LV_SYMBOL_LIST,
            "PRINT ONLY",
            UI_COLOR_BLUE,
            UI_COLOR_BLUE_DARK
        );

    lv_obj_set_width(
        btn_print,
        200
    );

    lv_obj_align(
        btn_print,
        LV_ALIGN_TOP_MID,
        0,
        145
    );

    lv_obj_add_event_cb(
        btn_print,
        result_print_cb,
        LV_EVENT_CLICKED,
        NULL
    );


    /* ---------------------------------------------------- */
    /* SAVE ONLY                                            */
    /* ---------------------------------------------------- */

    lv_obj_t * btn_save =
        ui_create_action_button(
            s_result_popup,
            LV_SYMBOL_SD_CARD,
            "SAVE ONLY",
            UI_COLOR_TEXT_DARK,
            UI_COLOR_TEXT_MUTED
        );

    lv_obj_set_width(
        btn_save,
        200
    );

    lv_obj_align(
        btn_save,
        LV_ALIGN_TOP_MID,
        0,
        195
    );

    lv_obj_add_event_cb(
        btn_save,
        result_save_cb,
        LV_EVENT_CLICKED,
        NULL
    );
}

static void status_card_cb(lv_event_t * e);
static void show_timer_popup();
static void timer_popup_close_cb(lv_event_t * e);
static void timer_set_cb(lv_event_t * e);

static void timer_popup_close()
{
    if (s_timer_popup)
    {
        lv_obj_del(s_timer_popup);

        s_timer_popup = NULL;
        s_timer_textarea = NULL;
        s_timer_keyboard = NULL;
    }
}

static void timer_popup_close_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    timer_popup_close();
}

static void start_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    home_start_sampling();
}

static void pause_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    home_pause_sampling();
}

static void stop_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    home_stop_sampling();

    show_result_popup();
}

static void status_card_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    show_timer_popup();
}

static void timer_set_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    if (!s_timer_textarea)
        return;

    const char * text =
        lv_textarea_get_text(s_timer_textarea);

    if (!text || text[0] == '\0')
        return;

    unsigned long minutes =
        strtoul(text, NULL, 10);

    if (minutes == 0)
        return;

    uint64_t seconds64 =
        (uint64_t)minutes * 60ULL;

    if (seconds64 > 0xFFFFFFFFULL)
        return;

    uint32_t seconds =
        (uint32_t)seconds64;

    if (home_set_sampling_duration(seconds))
    {
        timer_popup_close();
    }
}

static void show_timer_popup()
{
    if (s_timer_popup)
        return;

    s_timer_popup = lv_obj_create(lv_layer_top());

    lv_obj_set_size(
        s_timer_popup,
        520,
        450
    );

    lv_obj_center(s_timer_popup);

    lv_obj_set_style_radius(
        s_timer_popup,
        16,
        0
    );

    lv_obj_set_style_bg_color(
        s_timer_popup,
        lv_color_white(),
        0
    );

    lv_obj_set_style_bg_opa(
        s_timer_popup,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        s_timer_popup,
        2,
        0
    );

    lv_obj_set_style_border_color(
        s_timer_popup,
        UI_COLOR_TEXT_MUTED,
        0
    );

    lv_obj_clear_flag(
        s_timer_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );


    /* Title */
    lv_obj_t * title =
        lv_label_create(s_timer_popup);

    lv_label_set_text(
        title,
        "SAMPLING SETTINGS"
    );

    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_set_style_text_color(
        title,
        UI_COLOR_TEXT_DARK,
        0
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        15
    );


    /* Description */
    lv_obj_t * description =
        lv_label_create(s_timer_popup);

    lv_label_set_text(
        description,
        "Set sampling duration"
    );

    lv_obj_set_style_text_font(
        description,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_style_text_color(
        description,
        UI_COLOR_TEXT_MUTED,
        0
    );

    lv_obj_align(
        description,
        LV_ALIGN_TOP_MID,
        0,
        50
    );

    /* ---------------------------------------------------- */
    /* Duration input                                       */
    /* ---------------------------------------------------- */

    uint32_t current_seconds =
        home_get_sampling_duration();

    uint32_t current_minutes =
        current_seconds / 60U;

    char duration_text[16];

    snprintf(
        duration_text,
        sizeof(duration_text),
        "%lu",
        (unsigned long)current_minutes
    );


    /* Textarea */
    s_timer_textarea =
        lv_textarea_create(s_timer_popup);

    lv_obj_set_size(
        s_timer_textarea,
        220,
        50
    );

    lv_obj_align(
        s_timer_textarea,
        LV_ALIGN_TOP_MID,
        0,
        80
    );

    lv_textarea_set_one_line(
        s_timer_textarea,
        true
    );

    lv_textarea_set_accepted_chars(
        s_timer_textarea,
        "0123456789"
    );

    lv_textarea_set_text(
        s_timer_textarea,
        duration_text
    );

    lv_textarea_set_cursor_pos(
        s_timer_textarea,
        LV_TEXTAREA_CURSOR_LAST
    );

    lv_obj_set_style_text_font(
        s_timer_textarea,
        &lv_font_montserrat_20,
        0
    );

    /* SET button */
    lv_obj_t * btn_set =
        lv_btn_create(s_timer_popup);

    lv_obj_set_size(
        btn_set,
        120,
        45
    );

    lv_obj_align(
        btn_set,
        LV_ALIGN_TOP_MID,
        -70,
        145
    );

    lv_obj_add_event_cb(
        btn_set,
        timer_set_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t * set_label =
        lv_label_create(btn_set);

    lv_label_set_text(
        set_label,
        "SET"
    );

    lv_obj_center(set_label);

    /* CLOSE button */
    lv_obj_t * close_btn =
        lv_btn_create(s_timer_popup);

    lv_obj_set_size(
        close_btn,
        120,
        45
    );

    lv_obj_align(
        close_btn,
        LV_ALIGN_TOP_MID,
        70,
        145
    );

    lv_obj_add_event_cb(
        close_btn,
        timer_popup_close_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t * close_label =
        lv_label_create(close_btn);

    lv_label_set_text(
        close_label,
        "CLOSE"
    );

    lv_obj_center(close_label);

    /* ---------------------------------------------------- */
    /* Numeric keyboard                                     */
    /* ---------------------------------------------------- */

    s_timer_keyboard =
        lv_keyboard_create(s_timer_popup);

    lv_obj_set_size(
        s_timer_keyboard,
        460,
        180
    );

    lv_obj_align(
        s_timer_keyboard,
        LV_ALIGN_BOTTOM_MID,
        0,
        -10
    );

    lv_keyboard_set_mode(
        s_timer_keyboard,
        LV_KEYBOARD_MODE_NUMBER
    );

    lv_keyboard_set_textarea(
        s_timer_keyboard,
        s_timer_textarea
    );
}

/* ------------------------------------------------------------------------ */
/*  Row 1: FLOW RATE / TOTAL VOLUME / STATUS                                */
/* ------------------------------------------------------------------------ */
static lv_obj_t * build_flow_rate_card(lv_obj_t * parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t * card = ui_create_card(parent, w, h);
    lv_obj_set_style_bg_color(card, UI_COLOR_EDITABLE_CARD, 0);
    lv_obj_set_style_border_color(card, UI_COLOR_EDITABLE_BORDER, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 6, 0);

    lv_obj_add_flag(
        card,
        LV_OBJ_FLAG_CLICKABLE
    );

    lv_obj_add_event_cb(
        card,
        flow_card_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    /* Header row: "FLOW RATE" + green OK dot */
    lv_obj_t * header = lv_obj_create(card);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text(title, "FLOW RATE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MUTED, 0);

    lv_obj_t * status_group = lv_obj_create(header);
    lv_obj_remove_style_all(status_group);
    lv_obj_set_size(status_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(status_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status_group, 4, 0);

    s_lbl_flow_status_dot = lv_obj_create(status_group);
    lv_obj_remove_style_all(s_lbl_flow_status_dot);
    lv_obj_set_size(s_lbl_flow_status_dot, 10, 10);
    lv_obj_set_style_radius(s_lbl_flow_status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_lbl_flow_status_dot, UI_COLOR_GREEN, 0);
    lv_obj_set_style_bg_opa(s_lbl_flow_status_dot, LV_OPA_COVER, 0);

    lv_obj_t * ok_lbl = lv_label_create(status_group);
    lv_label_set_text(ok_lbl, "OK");
    lv_obj_set_style_text_font(ok_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ok_lbl, UI_COLOR_GREEN, 0);

    /* Big value row: "2000 LPM" */
    lv_obj_t * value_row = lv_obj_create(card);
    lv_obj_remove_style_all(value_row);
    lv_obj_set_size(value_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(value_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(value_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(value_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(value_row, 6, 0);

    s_lbl_flow_value = lv_label_create(value_row);
    lv_label_set_text(s_lbl_flow_value, "N/A");
    lv_obj_set_style_text_font(s_lbl_flow_value, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(s_lbl_flow_value, UI_COLOR_GREEN, 0);

    lv_obj_t * unit_lbl = lv_label_create(value_row);
    lv_label_set_text(unit_lbl, "LPM");
    lv_obj_set_style_text_font(unit_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(unit_lbl, UI_COLOR_TEXT_MUTED, 0);

    /* Setpoint caption */
    lv_obj_t * setpoint_row = lv_obj_create(card);
    lv_obj_remove_style_all(setpoint_row);
    lv_obj_set_size(setpoint_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(setpoint_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(setpoint_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(setpoint_row, 4, 0);

    lv_obj_t * sp_caption = lv_label_create(setpoint_row);
    lv_label_set_text(sp_caption, "Setpoint");
    lv_obj_set_style_text_font(sp_caption, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sp_caption, UI_COLOR_TEXT_MUTED, 0);

    s_lbl_flow_setpoint =
        lv_label_create(setpoint_row);

    lv_label_set_text(
        s_lbl_flow_setpoint,
        "2000 LPM"
    );

    lv_obj_set_style_text_font(
        s_lbl_flow_setpoint,
        &lv_font_montserrat_12,
        0
    );

    lv_obj_set_style_text_color(
        s_lbl_flow_setpoint,
        UI_COLOR_BLUE,
        0
    );

    /* Progress bar. Range remains 0..2000; endpoint labels are intentionally hidden. */
    s_bar_flow = lv_bar_create(card);
    lv_obj_set_size(s_bar_flow, lv_pct(100), 10);
    lv_bar_set_range(s_bar_flow, 0, 2000);
    lv_bar_set_value(s_bar_flow, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_flow, lv_color_hex(0xE5E7EB), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar_flow, UI_COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_bar_flow, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(s_bar_flow, 4, LV_PART_INDICATOR);

    uint32_t child_count =
        lv_obj_get_child_cnt(card);

    for (uint32_t i = 0; i < child_count; i++)
    {
        lv_obj_t * child =
            lv_obj_get_child(card, i);

        lv_obj_add_flag(
            child,
            LV_OBJ_FLAG_EVENT_BUBBLE
        );
    }

    return card;
}

static lv_obj_t * build_total_volume_card(lv_obj_t * parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t * card = ui_create_card(parent, w, h);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 6, 0);

    lv_obj_t * title = lv_label_create(card);
    lv_label_set_text(title, "TOTAL VOLUME");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MUTED, 0);

    lv_obj_t * value_row = lv_obj_create(card);
    lv_obj_remove_style_all(value_row);
    lv_obj_set_size(value_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(value_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(value_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(value_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(value_row, 6, 0);

    s_lbl_total_volume = lv_label_create(value_row);
    lv_label_set_text(s_lbl_total_volume, "N/A");
    lv_obj_set_style_text_font(s_lbl_total_volume, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(s_lbl_total_volume, UI_COLOR_TEXT_DARK, 0);

    lv_obj_t * unit_lbl = lv_label_create(value_row);
    lv_label_set_text(unit_lbl, "m3");
    lv_obj_set_style_text_font(unit_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(unit_lbl, UI_COLOR_TEXT_MUTED, 0);

    lv_obj_t * instant_row = lv_obj_create(card);
    lv_obj_remove_style_all(instant_row);
    lv_obj_set_size(instant_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(instant_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(instant_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(instant_row, 6, 0);

    lv_obj_t * instant_caption = lv_label_create(instant_row);
    lv_label_set_text(instant_caption, "Instant Flow");
    lv_obj_set_style_text_font(instant_caption, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(instant_caption, UI_COLOR_TEXT_MUTED, 0);

    s_lbl_instant_flow = lv_label_create(instant_row);
    lv_label_set_text(s_lbl_instant_flow, "N/A");
    lv_obj_set_style_text_font(s_lbl_instant_flow, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_lbl_instant_flow, UI_COLOR_BLUE, 0);

    return card;
}

static lv_obj_t * build_status_card(lv_obj_t * parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t * card = ui_create_card(parent, w, h);
    lv_obj_set_style_bg_color(card, UI_COLOR_EDITABLE_CARD, 0);
    lv_obj_set_style_border_color(card, UI_COLOR_EDITABLE_BORDER, 0);

    lv_obj_add_flag(
        card,
        LV_OBJ_FLAG_CLICKABLE
    );

    lv_obj_add_event_cb(
        card,
        status_card_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 2, 0);

    lv_obj_t * title = lv_label_create(card);
    lv_label_set_text(title, "STATUS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MUTED, 0);

    s_lbl_run_status = lv_label_create(card);
    lv_label_set_text(s_lbl_run_status, "STOPPED");
    lv_obj_set_style_text_font(s_lbl_run_status, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_lbl_run_status, UI_COLOR_GREEN, 0);

    lv_obj_t * remaining_caption = lv_label_create(card);
    lv_label_set_text(remaining_caption, "REMAINING TIME");
    lv_obj_set_style_text_font(
        remaining_caption,
        &lv_font_montserrat_12,
        0
    );
    lv_obj_set_style_text_color(
        remaining_caption,
        UI_COLOR_TEXT_MUTED,
        0
    );
    lv_obj_set_style_pad_top(
        remaining_caption,
        3,
        0
    );

    s_lbl_remaining_time = lv_label_create(card);
    lv_label_set_text(
        s_lbl_remaining_time,
        "24:00:00"
    );
    lv_obj_set_style_text_font(
        s_lbl_remaining_time,
        &lv_font_montserrat_20,
        0
    );
    lv_obj_set_style_text_color(
        s_lbl_remaining_time,
        UI_COLOR_TEXT_DARK,
        0
    );

    /* Allow touch events from child labels to reach the Status Card */
    uint32_t child_count = lv_obj_get_child_cnt(card);

    for (uint32_t i = 0; i < child_count; i++)
    {
        lv_obj_t * child = lv_obj_get_child(card, i);

        lv_obj_add_flag(
            child,
            LV_OBJ_FLAG_EVENT_BUBBLE
        );
    }

    return card;
}

/* ------------------------------------------------------------------------ */
/*  Row 2: sensor stat tiles (TEMP / HUMIDITY / PRESSURE / WIND SPEED / DIR) */
/* ------------------------------------------------------------------------ */
static lv_obj_t * build_sensor_row(lv_obj_t * parent, lv_coord_t w)
{
    lv_obj_t * card = ui_create_card(parent, w, 72);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_val_temp     = ui_create_stat_tile(card, LV_SYMBOL_CHARGE, "--", "C",   "TEMP");
    s_val_humidity = ui_create_stat_tile(card, LV_SYMBOL_TINT,   "--", "%RH", "HUMIDITY");
    s_val_pressure = ui_create_stat_tile(card, LV_SYMBOL_DOWNLOAD, "--", "hPa", "PRESSURE");
    s_val_wind_speed = ui_create_stat_tile(card, LV_SYMBOL_SHUFFLE, "N/A", "m/s", "WIND SPEED");
    s_val_wind_dir   = ui_create_stat_tile(card, LV_SYMBOL_GPS, "N/A", "", "WIND DIR.");

    return card;
}

/* ------------------------------------------------------------------------ */
/*  Row 3: device indicators (FILTER / SD CARD / PRINTER / POWER)           */
/* ------------------------------------------------------------------------ */
static lv_obj_t * build_indicator_tile(lv_obj_t * parent, const char * symbol,
                                        const char * caption, const char * status, lv_color_t status_color)
{
    lv_obj_t * tile = lv_obj_create(parent);
    lv_obj_remove_style_all(tile);
    lv_obj_set_size(tile, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(tile, 1);
    lv_obj_set_layout(tile, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tile, 8, 0);

    lv_obj_t * icon = lv_label_create(tile);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_TEXT_MUTED, 0);

    lv_obj_t * col = lv_obj_create(tile);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * caption_lbl = lv_label_create(col);
    lv_label_set_text(caption_lbl, caption);
    lv_obj_set_style_text_font(caption_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(caption_lbl, UI_COLOR_TEXT_MUTED, 0);

    lv_obj_t * status_lbl = lv_label_create(col);
    lv_label_set_text(status_lbl, status);
    lv_obj_set_style_text_font(status_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_lbl, status_color, 0);

    return status_lbl;
}

static lv_obj_t * build_indicator_row(lv_obj_t * parent, lv_coord_t w)
{
    lv_obj_t * card = ui_create_card(parent, w, 56);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_lbl_filter =
        build_indicator_tile(
            card,
            LV_SYMBOL_WARNING,
            "FILTER",
            "N/A",
            UI_COLOR_TEXT_MUTED
        );

    s_lbl_sd =
        build_indicator_tile(
            card,
            LV_SYMBOL_SD_CARD,
            "SD CARD",
            "N/A",
            UI_COLOR_TEXT_MUTED
        );

    s_lbl_printer =
        build_indicator_tile(
            card,
            LV_SYMBOL_LIST,
            "PRINTER",
            "N/A",
            UI_COLOR_TEXT_MUTED
        );

    s_lbl_power =
        build_indicator_tile(
            card,
            LV_SYMBOL_CHARGE,
            "POWER",
            "N/A",
            UI_COLOR_TEXT_MUTED
        );

    return card;
}

/* ------------------------------------------------------------------------ */
/*  Row 4: START / PAUSE / STOP                                             */
/* ------------------------------------------------------------------------ */
static lv_obj_t * build_action_row(lv_obj_t * parent, lv_coord_t w)
{
    lv_obj_t * row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, w, 56);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    s_btn_start = ui_create_action_button(row, LV_SYMBOL_PLAY, "START", UI_COLOR_GREEN, UI_COLOR_GREEN_DARK);
    lv_obj_add_event_cb(s_btn_start, start_btn_cb, LV_EVENT_CLICKED, NULL);

    s_btn_pause = ui_create_action_button(row, LV_SYMBOL_PAUSE, "PAUSE", UI_COLOR_BLUE, UI_COLOR_BLUE_DARK);
    lv_obj_add_event_cb(s_btn_pause, pause_btn_cb, LV_EVENT_CLICKED, NULL);

    s_btn_stop = ui_create_action_button(row, LV_SYMBOL_STOP, "STOP", UI_COLOR_RED, UI_COLOR_RED_DARK);
    lv_obj_add_event_cb(s_btn_stop, stop_btn_cb, LV_EVENT_CLICKED, NULL);

    return row;
}

/* ------------------------------------------------------------------------ */
/*  Public entry point                                                       */
/* ------------------------------------------------------------------------ */
lv_obj_t * screen_home_create(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, UI_COLOR_PAGE_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_sidebar(scr, UI_PAGE_HOME);
    ui_create_home_topbar(scr);

    /* Content container to the right of the sidebar, below the top bar */
    lv_obj_t * content = lv_obj_create(scr);
    lv_obj_remove_style_all(content);
    lv_obj_set_pos(content, UI_SIDEBAR_WIDTH, UI_TOPBAR_HEIGHT);
    lv_obj_set_size(content, UI_HOR_RES - UI_SIDEBAR_WIDTH, UI_VER_RES - UI_TOPBAR_HEIGHT);
    lv_obj_set_style_pad_all(content, 12, 0);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_coord_t full_w = UI_HOR_RES - UI_SIDEBAR_WIDTH - 24; /* minus content padding */

    /* Row 1: three cards side by side */
    lv_obj_t * row1 = lv_obj_create(content);
    lv_obj_remove_style_all(row1);
    lv_obj_set_size(row1, full_w, 160);
    lv_obj_set_layout(row1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row1, 10, 0);
    lv_obj_clear_flag(row1, LV_OBJ_FLAG_SCROLLABLE);

lv_coord_t col_w = (full_w - 20) / 3;

build_flow_rate_card(row1, col_w, 160);
build_total_volume_card(row1, col_w, 160);
build_status_card(row1, col_w, 160);

    build_sensor_row(content, full_w);
    build_indicator_row(content, full_w);
    build_action_row(content, full_w);

    return scr;
}

void screen_home_show_result_popup()
{
    show_result_popup();
}

uint32_t screen_home_get_flow_setpoint()
{
    return s_flow_setpoint;
}

void screen_home_update_device_status(
    bool filter_ready,
    bool sd_ready,
    bool printer_ready
)
{
    if (s_lbl_filter)
    {
        lv_label_set_text(
            s_lbl_filter,
            filter_ready ? "READY" : "N/A"
        );

        lv_obj_set_style_text_color(
            s_lbl_filter,
            filter_ready
                ? UI_COLOR_GREEN
                : UI_COLOR_TEXT_MUTED,
            0
        );
    }

    if (s_lbl_sd)
    {
        lv_label_set_text(
            s_lbl_sd,
            sd_ready ? "READY" : "N/A"
        );

        lv_obj_set_style_text_color(
            s_lbl_sd,
            sd_ready
                ? UI_COLOR_GREEN
                : UI_COLOR_TEXT_MUTED,
            0
        );
    }

    if (s_lbl_printer)
    {
        lv_label_set_text(
            s_lbl_printer,
            printer_ready ? "READY" : "N/A"
        );

        lv_obj_set_style_text_color(
            s_lbl_printer,
            printer_ready
                ? UI_COLOR_GREEN
                : UI_COLOR_TEXT_MUTED,
            0
        );
    }
}

/* ------------------------------------------------------------------------ */
/*  Live-data update API - call these from your sensor/control task          */
/* ------------------------------------------------------------------------ */
void screen_home_update_flow(float lpm)
{
    if (s_lbl_flow_value)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", lpm);
        lv_label_set_text(s_lbl_flow_value, buf);
    }

    if (s_bar_flow)
    {
        int32_t value = (int32_t)lpm;

        if (value < 0)
            value = 0;

        if (value > 2000)
            value = 2000;

        lv_bar_set_value(
            s_bar_flow,
            value,
            LV_ANIM_ON
        );
    }
}

void screen_home_update_volume(float total_m3, float instant_m3_min)
{
    char buf[24];

    if (s_lbl_total_volume)
    {
        snprintf(buf, sizeof(buf), "%.2f", total_m3);
        lv_label_set_text(s_lbl_total_volume, buf);
    }

    if (s_lbl_instant_flow)
    {
        snprintf(buf, sizeof(buf), "%.3f m3/min", instant_m3_min);
        lv_label_set_text(s_lbl_instant_flow, buf);
    }
}

void screen_home_update_power(float ac_voltage, bool connected)
{
    if (!s_lbl_power) return;

    char buf[24];
    if (connected)
    {
        snprintf(buf, sizeof(buf), "%.0f VAC", ac_voltage);
        lv_label_set_text(s_lbl_power, buf);
        lv_obj_set_style_text_color(s_lbl_power, UI_COLOR_TEXT_DARK, 0);
    }
    else
    {
        lv_label_set_text(s_lbl_power, "N/A");
        lv_obj_set_style_text_color(s_lbl_power, UI_COLOR_TEXT_MUTED, 0);
    }
}

void screen_home_update_environment(float temp_c, float rh_pct, float pressure_hpa,
                                     float wind_speed_ms, int wind_dir_deg, const char * wind_dir_cardinal)
{
    char buf[24];
    if (s_val_temp) { snprintf(buf, sizeof(buf), "%.1f", temp_c); lv_label_set_text(s_val_temp, buf); }
    if (s_val_humidity) { snprintf(buf, sizeof(buf), "%.1f", rh_pct); lv_label_set_text(s_val_humidity, buf); }
    if (s_val_pressure) { snprintf(buf, sizeof(buf), "%.1f", pressure_hpa); lv_label_set_text(s_val_pressure, buf); }
    if (s_val_wind_speed)
    {
        snprintf(
            buf,
            sizeof(buf),
            "%.1f",
            wind_speed_ms
        );

        lv_label_set_text(
            s_val_wind_speed,
            buf
        );
    }
    if (s_val_wind_dir)
    {
        if (wind_dir_cardinal && wind_dir_cardinal[0])
        {
            snprintf(
                buf,
                sizeof(buf),
                "%d %s",
                wind_dir_deg,
                wind_dir_cardinal
            );
        }
        else
        {
            snprintf(buf, sizeof(buf), "N/A");
        }

        lv_label_set_text(
            s_val_wind_dir,
            buf
        );
    }
}

void screen_home_update_timers(const char * sampling_timer_hms, const char * remaining_time_hms)
{
    if (s_lbl_sampling_timer && sampling_timer_hms)
        lv_label_set_text(s_lbl_sampling_timer, sampling_timer_hms);
    if (s_lbl_remaining_time && remaining_time_hms)
        lv_label_set_text(s_lbl_remaining_time, remaining_time_hms);
}

void screen_home_update_status(const char *status)
{
    if (!s_lbl_run_status || !status)
        return;

    lv_label_set_text(s_lbl_run_status, status);

    if (strcmp(status, "RUNNING") == 0)
    {
        lv_obj_set_style_text_color(
            s_lbl_run_status,
            UI_COLOR_GREEN,
            0
        );
    }
    else if (strcmp(status, "PAUSED") == 0)
    {
        lv_obj_set_style_text_color(
            s_lbl_run_status,
            UI_COLOR_BLUE,
            0
        );
    }
    else if (strcmp(status, "FINISHED") == 0)
    {
        lv_obj_set_style_text_color(
            s_lbl_run_status,
            UI_COLOR_BLUE,
            0
        );
    }
    else
    {
        lv_obj_set_style_text_color(
            s_lbl_run_status,
            UI_COLOR_RED,
            0
        );
    }

    bool running =
        (strcmp(status, "RUNNING") == 0);

    bool active =
        running ||
        (strcmp(status, "PAUSED") == 0);

    if (s_btn_start)
    {
        if (running)
            lv_obj_add_state(
                s_btn_start,
                LV_STATE_DISABLED
            );
        else
            lv_obj_clear_state(
                s_btn_start,
                LV_STATE_DISABLED
            );
    }

    if (s_btn_pause)
    {
        if (running)
            lv_obj_clear_state(
                s_btn_pause,
                LV_STATE_DISABLED
            );
        else
            lv_obj_add_state(
                s_btn_pause,
                LV_STATE_DISABLED
            );
    }

    if (s_btn_stop)
    {
        if (active)
            lv_obj_clear_state(
                s_btn_stop,
                LV_STATE_DISABLED
            );
        else
            lv_obj_add_state(
                s_btn_stop,
                LV_STATE_DISABLED
            );
    }
}