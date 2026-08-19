#pragma once

#include <Arduino.h>

enum SamplingState
{
    SAMPLING_IDLE = 0,
    SAMPLING_RUNNING,
    SAMPLING_PAUSED,
    SAMPLING_FINISHED
};

struct SamplingResult
{
    bool valid;

    String datetime;
    uint32_t duration_seconds;

    float temperature;
    float humidity;
    float pressure;

    float ac_voltage;

    float wind_speed;
    int wind_direction;
    String wind_cardinal;
};

extern SamplingState samplingState;
extern SamplingResult lastSamplingResult;

void sampling_init();
void sampling_start();
void sampling_pause();
void sampling_stop();
void sampling_task();

bool sampling_set_duration(uint32_t seconds);
uint32_t sampling_get_duration();
uint32_t sampling_get_elapsed_seconds();

String getElapsedTime();
String getRemainingTime();
SamplingState getSamplingState();

const SamplingResult &getLastSamplingResult();
