/**
 * ui.c
 * Top-level screen manager: creates screens lazily on first visit and
 * handles lv_scr_load_anim() transitions between them.
 *
 * Call ui_init() once after lv_init() + your display/touch driver are ready,
 * then keep calling lv_timer_handler() in your main loop as usual.
 */
#include "ui_common.h"
#include "ui_screens.h"

static lv_obj_t * s_screens[UI_PAGE_COUNT] = { NULL };

static lv_obj_t * create_screen(ui_page_id_t page)
{
    switch (page) {
        case UI_PAGE_HOME:        return screen_home_create();
        case UI_PAGE_SAMPLING:    return screen_sampling_create();
        case UI_PAGE_TREND:       return screen_trend_create();
        case UI_PAGE_DATA:        return screen_data_create();
        case UI_PAGE_SYSTEM:      return screen_system_create();
        case UI_PAGE_CALIBRATION: return screen_calibration_create();
        default:                  return NULL;
    }
}

/* Called by ui_common.c's ui_navigate_to() wrapper */
void ui_screen_manager_navigate(ui_page_id_t page)
{
    if (page >= UI_PAGE_COUNT) return;

    if (s_screens[page] == NULL) {
        s_screens[page] = create_screen(page);
    }
    if (s_screens[page] == NULL) return;

    //lv_scr_load_anim(s_screens[page], LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
    lv_scr_load(s_screens[page]);
}

void ui_init(void)
{
    /* Home is built eagerly since it's the boot screen; the rest are built
     * on demand the first time the user navigates to them, which keeps RAM
     * usage down on the ESP32-S3 (each screen + its widgets costs memory). */
    s_screens[UI_PAGE_HOME] = screen_home_create();
    lv_scr_load(s_screens[UI_PAGE_HOME]);
}
