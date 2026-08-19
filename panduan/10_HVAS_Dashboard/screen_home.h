#pragma once

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_home_create(void);

void screen_home_update_flow(float lpm);
uint32_t screen_home_get_flow_setpoint();

void screen_home_update_volume(
    float total_m3,
    float instant_m3_min
);

void screen_home_update_power(
    float ac_voltage,
    bool connected
);

void screen_home_update_device_status(
    bool filter_ready,
    bool sd_ready,
    bool printer_ready
);

void screen_home_update_environment(
    float temp_c,
    float rh_pct,
    float pressure_hpa,
    float wind_speed_ms,
    int wind_dir_deg,
    const char *wind_dir_cardinal
);

void screen_home_update_timers(
    const char *sampling_timer_hms,
    const char *remaining_time_hms
);

void screen_home_update_status(
    const char *status
);

void screen_home_show_result_popup();

#ifdef __cplusplus
}
#endif