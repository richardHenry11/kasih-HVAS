#include "home_controller.h"
#include "communication.h"
#include "screen_home.h"
#include "sampling_controller.h"
#include "ui_common.h"
#include "ui_screens.h"
#include "lvgl_v8_port.h"
#include "printer_controller.h"
#include "sd_controller.h"

#include <stdio.h>
#include <string.h>

static uint32_t lastHomeUpdate = 0;

static void updateEnvironment();
static void updateRTC();
static void updateVoltage();
static void updateGPS();
static void updateFlow();
static void updateVolume();
static void updateTimer();
static SamplingState lastObservedSamplingState = SAMPLING_IDLE;

void home_init()
{
    lastHomeUpdate = 0;
}

void home_update()
{
    if (millis() - lastHomeUpdate < 500)
        return;

    lastHomeUpdate = millis();

    /* LVGL is also serviced by the display task. Protect all UI updates. */
    lvgl_port_lock(-1);

    updateEnvironment();
    updateRTC();
    updateVoltage();
    screen_home_update_device_status(true, sd_controller_is_ready(), printer_is_ready());
    updateGPS();
    updateFlow();
    updateVolume();
    updateTimer();

    /*
     * Automatic completion must use the same result popup as a manual STOP.
     * Manual STOP opens it from the button callback, while automatic finish
     * changes the sampling state inside sampling_task(). Detect that state
     * transition here and open the popup exactly once.
     */
    const SamplingState currentState = getSamplingState();
    if (currentState == SAMPLING_FINISHED &&
        lastObservedSamplingState != SAMPLING_FINISHED)
    {
        screen_home_show_result_popup();
    }
    lastObservedSamplingState = currentState;

    lvgl_port_unlock();
}

static void updateEnvironment()
{
    if (!systemInfo.bme_connected)
        return;

    /* Wind values stay N/A until the UART2 wind sensor is integrated. */
    screen_home_update_environment(
        systemInfo.temperature,
        systemInfo.humidity,
        systemInfo.pressure,
        0.0f,
        0,
        ""
    );
}

static void updateRTC()
{
    if (!systemInfo.rtc_connected || systemInfo.datetime.length() < 19)
    {
        ui_home_topbar_update_datetime("--", "--:--:--");
        screen_system_update("N/A", systemInfo.device.c_str(), systemInfo.version.c_str());
        return;
    }

    const String &dt = systemInfo.datetime;

    char dateBuf[24];
    char timeBuf[16];

    /* Expected ATmega format: YYYY-MM-DD HH:MM:SS */
    int year = dt.substring(0, 4).toInt();
    int month = dt.substring(5, 7).toInt();
    int day = dt.substring(8, 10).toInt();

    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    if (year >= 2000 && month >= 1 && month <= 12 && day >= 1 && day <= 31)
        snprintf(dateBuf, sizeof(dateBuf), "%02d %s %04d", day, months[month - 1], year);
    else
        snprintf(dateBuf, sizeof(dateBuf), "N/A");

    snprintf(timeBuf, sizeof(timeBuf), "%s", dt.substring(11, 19).c_str());

    ui_home_topbar_update_datetime(dateBuf, timeBuf);
    screen_system_update(dt.c_str(), systemInfo.device.c_str(), systemInfo.version.c_str());
}

static void updateVoltage()
{
    /* The Home card is labeled POWER 220 VAC, so use PZEM AC voltage. */
    screen_home_update_power(systemInfo.ac_voltage, systemInfo.pzem_connected);
}

static void updateGPS()
{
    char latBuf[32];
    char lngBuf[32];

    if (systemInfo.gps_connected && systemInfo.gps_fix)
    {
        snprintf(latBuf, sizeof(latBuf), "Lat  %.6f", systemInfo.latitude);
        snprintf(lngBuf, sizeof(lngBuf), "Long %.6f", systemInfo.longitude);
    }
    else
    {
        snprintf(latBuf, sizeof(latBuf), "Lat  --");
        snprintf(lngBuf, sizeof(lngBuf), "Long --");
    }

    ui_home_topbar_update_gps(latBuf, lngBuf);
}

static void updateFlow()
{
    /* TEMPORARY SIMULATION: until the ATmega flow protocol is available,
     * use the configured setpoint as the displayed actual flow. */
    const float simulatedActualLpm = (float)screen_home_get_flow_setpoint();
    screen_home_update_flow(simulatedActualLpm);
}

static void updateVolume()
{
    /* TEMPORARY SIMULATION: volume = actual flow * elapsed time.
     * LPM -> m3/min: divide by 1000.
     * total m3 = LPM * seconds / 60000. */
    static float totalVolumeM3 = 0.0f;
    static SamplingState previousState = SAMPLING_IDLE;

    const SamplingState state = getSamplingState();
    const float actualLpm = (float)screen_home_get_flow_setpoint();

    if (state == SAMPLING_RUNNING &&
        (previousState == SAMPLING_IDLE || previousState == SAMPLING_FINISHED))
    {
        totalVolumeM3 = 0.0f;
    }

    if (state == SAMPLING_RUNNING ||
        state == SAMPLING_PAUSED ||
        state == SAMPLING_FINISHED)
    {
        totalVolumeM3 =
            (actualLpm * (float)sampling_get_elapsed_seconds()) / 60000.0f;
    }
    else if (state == SAMPLING_IDLE &&
             previousState == SAMPLING_IDLE)
    {
        totalVolumeM3 = 0.0f;
    }

    screen_home_update_volume(
        totalVolumeM3,
        actualLpm / 1000.0f
    );

    previousState = state;
}

static void updateTimer()
{
    String elapsed = getElapsedTime();
    String remaining = getRemainingTime();

    screen_home_update_timers(elapsed.c_str(), remaining.c_str());

    switch (getSamplingState())
    {
        case SAMPLING_IDLE:
            screen_home_update_status("STOPPED");
            break;

        case SAMPLING_RUNNING:
            screen_home_update_status("RUNNING");
            break;

        case SAMPLING_PAUSED:
            screen_home_update_status("PAUSED");
            break;

        case SAMPLING_FINISHED:
            screen_home_update_status("FINISHED");
            break;
    }
}

void home_start_sampling()
{
    sampling_start();
}

void home_pause_sampling()
{
    sampling_pause();
}

void home_stop_sampling()
{
    sampling_stop();
}

bool home_set_sampling_duration(uint32_t seconds)
{
    return sampling_set_duration(seconds);
}

uint32_t home_get_sampling_duration()
{
    return sampling_get_duration();
}

uint32_t home_get_flow_setpoint()
{
    return screen_home_get_flow_setpoint();
}
