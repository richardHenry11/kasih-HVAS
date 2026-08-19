/**
 * ui_screens.h
 * Declares the create-functions for every screen and the live-update API
 * each screen exposes so a sensor/control task can push real values in.
 */
#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- HOME ---- */
lv_obj_t * screen_home_create(void);
void screen_home_update_flow(float lpm);
void screen_home_update_volume(float total_m3, float instant_m3_min);
void screen_home_update_power(float ac_voltage, bool connected);
void screen_home_update_environment(float temp_c, float rh_pct, float pressure_hpa,
                                     float wind_speed_ms, int wind_dir_deg, const char * wind_dir_cardinal);
void screen_home_update_timers(const char * sampling_timer_hms, const char * remaining_time_hms);
void screen_home_update_status(const char * status);

/* ---- SAMPLING SETUP ---- */
lv_obj_t * screen_sampling_create(void);

/* ---- TREND ---- */
lv_obj_t * screen_trend_create(void);
/* Push a new flow-rate sample (LPM). The screen keeps a rolling window and
 * recomputes AVG/MIN/MAX automatically. */
void screen_trend_add_sample(float lpm);

/* ---- DATA LOGGER ---- */
typedef struct {
    char date[12];    /* "15/05/25" */
    char time[8];     /* "10:30"    */
    float volume_m3;
    bool ok;
} data_logger_entry_t;

lv_obj_t * screen_data_create(void);
/* Replace the currently displayed page of log rows (max 5 rows/page in this UI). */
void screen_data_set_rows(const data_logger_entry_t * entries, int count, int page_no, int page_total);

/* ---- CALIBRATION ---- */
lv_obj_t * screen_calibration_create(void);

/* ---- SYSTEM ---- */
lv_obj_t * screen_system_create(void);
void screen_system_update(const char *datetime, const char *device, const char *version);

#ifdef __cplusplus
}
#endif

#endif /* UI_SCREENS_H */
