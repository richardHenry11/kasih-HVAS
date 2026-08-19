/**
 * screen_data.c
 * DATA LOGGER page: table of logged samples (No/Date/Time/Volume/Status),
 * DETAIL / EXPORT CSV / DELETE actions, and page navigation.
 */
#include "ui_common.h"
#include "ui_screens.h"
#include <stdio.h>
#include <string.h>

#define DATA_ROWS_PER_PAGE 5
#define DATA_TABLE_COLS 5

static lv_obj_t * s_table;
static lv_obj_t * s_lbl_page;
static int s_selected_row = -1;

/* These callbacks just log intent - wire them to your actual SD-card /
 * CSV-export / delete-confirmation logic. */
static void detail_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    LV_LOG_USER("DETAIL requested for row %d", s_selected_row);
}

static void export_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    LV_LOG_USER("EXPORT CSV requested");
}

static void delete_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    LV_LOG_USER("DELETE requested for row %d (show a confirm dialog first!)", s_selected_row);
}

static void prev_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    LV_LOG_USER("Previous page requested (fetch it and call screen_data_set_rows)");
}

static void next_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    LV_LOG_USER("Next page requested (fetch it and call screen_data_set_rows)");
}

static void table_draw_event_cb(lv_event_t * e)
{
    /* Highlight the header row and color the STATUS column green for "OK". */
    lv_obj_t * table = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if (dsc->part != LV_PART_ITEMS) return;

    if (dsc->id / DATA_TABLE_COLS == 0) {
        /* Header row: the background rect and the label text arrive in two
         * SEPARATE draw-part events (rect_dsc is NULL during the text pass
         * and label_dsc is NULL during the background pass) - never touch
         * both unconditionally, always NULL-check first. */
        if (dsc->rect_dsc) {
            dsc->rect_dsc->bg_color = UI_COLOR_PAGE_BG;
        }
        if (dsc->label_dsc) {
            dsc->label_dsc->color = UI_COLOR_TEXT_MUTED;
            dsc->label_dsc->font = &lv_font_montserrat_12;
        }
    } else if (dsc->id % DATA_TABLE_COLS == (DATA_TABLE_COLS - 1)) {
        if (dsc->label_dsc) {
            const char * txt = lv_table_get_cell_value(table, dsc->id / DATA_TABLE_COLS, dsc->id % DATA_TABLE_COLS);
            dsc->label_dsc->color = (txt && strcmp(txt, "OK") == 0) ? UI_COLOR_GREEN : UI_COLOR_RED;
        }
    }
}

static void table_click_cb(lv_event_t * e)
{
    lv_obj_t * table = lv_event_get_target(e);
    uint16_t row, col;
    lv_table_get_selected_cell(table, &row, &col);
    if (row == 0 || row == LV_TABLE_CELL_NONE) return; /* header row not selectable */
    s_selected_row = row;
}

lv_obj_t * screen_data_create(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, UI_COLOR_PAGE_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_subpage_topbar(scr, "DATA LOGGER", UI_PAGE_HOME);

    lv_obj_t * card = ui_create_card(scr, UI_HOR_RES - 32, UI_VER_RES - UI_SUBPAGE_TOPBAR_H - 32);
    lv_obj_set_pos(card, 16, UI_SUBPAGE_TOPBAR_H + 16);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 10, 0);

    /* Table */
    s_table = lv_table_create(card);
    lv_obj_set_size(s_table, lv_pct(100), 220);
    lv_obj_set_style_border_width(s_table, 0, LV_PART_MAIN);
    lv_table_set_col_cnt(s_table, DATA_TABLE_COLS);
    lv_table_set_row_cnt(s_table, DATA_ROWS_PER_PAGE + 1); /* +1 for header row */
    lv_table_set_col_width(s_table, 0, 80);
    lv_table_set_col_width(s_table, 1, 150);
    lv_table_set_col_width(s_table, 2, 130);
    lv_table_set_col_width(s_table, 3, 200);
    lv_table_set_col_width(s_table, 4, 130);

    lv_table_set_cell_value(s_table, 0, 0, "No.");
    lv_table_set_cell_value(s_table, 0, 1, "Date");
    lv_table_set_cell_value(s_table, 0, 2, "Time");
    lv_table_set_cell_value(s_table, 0, 3, "Volume (m3)");
    lv_table_set_cell_value(s_table, 0, 4, "Status");

    /* Seed with the example rows from the mock-up; replace via screen_data_set_rows() */
    data_logger_entry_t demo_rows[DATA_ROWS_PER_PAGE] = {
        { "15/05/25", "10:30", 12450.0f, true },
        { "15/05/25", "09:30", 10450.0f, true },
        { "15/05/25", "08:30", 8350.0f,  true },
        { "15/05/25", "07:30", 6250.0f,  true },
        { "15/05/25", "06:30", 4150.0f,  true },
    };
    //screen_data_set_rows(demo_rows, DATA_ROWS_PER_PAGE, 1, 21);

    lv_obj_add_event_cb(s_table, table_draw_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_obj_add_event_cb(s_table, table_click_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Action button row: DETAIL / EXPORT CSV / DELETE */
    lv_obj_t * action_row = lv_obj_create(card);
    lv_obj_remove_style_all(action_row);
    lv_obj_set_size(action_row, lv_pct(100), 52);
    lv_obj_set_layout(action_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(action_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(action_row, 12, 0);
    lv_obj_clear_flag(action_row, LV_OBJ_FLAG_SCROLLABLE);

    const char * labels[3] = { LV_SYMBOL_FILE " DETAIL", LV_SYMBOL_UPLOAD " EXPORT CSV", LV_SYMBOL_TRASH " DELETE" };
    lv_event_cb_t cbs[3] = { detail_btn_cb, export_btn_cb, delete_btn_cb };
    for (int i = 0; i < 3; i++) {
        lv_obj_t * btn = lv_btn_create(action_row);
        lv_obj_set_style_bg_color(btn, UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_border_color(btn, UI_COLOR_CARD_BORDER, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_height(btn, 52);
        lv_obj_add_event_cb(btn, cbs[i], LV_EVENT_CLICKED, NULL);
        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_DARK, 0);
        lv_obj_center(lbl);
    }

    /* Pagination row */
    lv_obj_t * page_row = lv_obj_create(card);
    lv_obj_remove_style_all(page_row);
    lv_obj_set_size(page_row, lv_pct(100), 40);
    lv_obj_set_layout(page_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(page_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(page_row, 16, 0);
    lv_obj_clear_flag(page_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * prev_btn = lv_btn_create(page_row);
    lv_obj_set_size(prev_btn, 40, 40);
    lv_obj_set_style_bg_color(prev_btn, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_border_color(prev_btn, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(prev_btn, 1, 0);
    lv_obj_set_style_shadow_width(prev_btn, 0, 0);
    lv_obj_add_event_cb(prev_btn, prev_page_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * prev_lbl = lv_label_create(prev_btn);
    lv_label_set_text(prev_lbl, LV_SYMBOL_LEFT);
    lv_obj_center(prev_lbl);

    s_lbl_page = lv_label_create(page_row);
    lv_label_set_text(s_lbl_page, "1 / 21");
    lv_obj_set_style_text_font(s_lbl_page, &lv_font_montserrat_16, 0);

    lv_obj_t * next_btn = lv_btn_create(page_row);
    lv_obj_set_size(next_btn, 40, 40);
    lv_obj_set_style_bg_color(next_btn, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_border_color(next_btn, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(next_btn, 1, 0);
    lv_obj_set_style_shadow_width(next_btn, 0, 0);
    lv_obj_add_event_cb(next_btn, next_page_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, LV_SYMBOL_RIGHT);
    lv_obj_center(next_lbl);

    /*tambahan*/
    screen_data_set_rows(demo_rows, DATA_ROWS_PER_PAGE, 1, 21);

    return scr;
}

/* ------------------------------------------------------------------------ */
/*  Data-binding API                                                         */
/* ------------------------------------------------------------------------ */
void screen_data_set_rows(const data_logger_entry_t * entries, int count, int page_no, int page_total)
{
    if (!s_table) return;
    if (count > DATA_ROWS_PER_PAGE) count = DATA_ROWS_PER_PAGE;

    /* Log numbers count down from the newest entry, matching the mock-up (125..121) */
    int base_no = 121 + (page_no - 1) * DATA_ROWS_PER_PAGE * 0; /* caller supplies real numbering if needed */
    LV_UNUSED(base_no);

    for (int i = 0; i < count; i++) {
        char no_buf[8], vol_buf[16];
        snprintf(no_buf, sizeof(no_buf), "%d", 125 - i - (page_no - 1) * DATA_ROWS_PER_PAGE);
        snprintf(vol_buf, sizeof(vol_buf), "%.1f", entries[i].volume_m3);

        lv_table_set_cell_value(s_table, i + 1, 0, no_buf);
        lv_table_set_cell_value(s_table, i + 1, 1, entries[i].date);
        lv_table_set_cell_value(s_table, i + 1, 2, entries[i].time);
        lv_table_set_cell_value(s_table, i + 1, 3, vol_buf);
        lv_table_set_cell_value(s_table, i + 1, 4, entries[i].ok ? "OK" : "FAULT");
    }

    char page_buf[24];
    snprintf(page_buf, sizeof(page_buf), "%d / %d", page_no, page_total);
    lv_label_set_text(s_lbl_page, page_buf);
}
