#pragma once

#include <stdint.h>

void home_init();
void home_update();

void home_start_sampling();
void home_pause_sampling();
void home_stop_sampling();

bool home_set_sampling_duration(uint32_t seconds);
uint32_t home_get_sampling_duration();
uint32_t home_get_flow_setpoint();
