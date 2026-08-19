#include "sampling_controller.h"
#include "communication.h"
#include <ArduinoJson.h>

SamplingState samplingState = SAMPLING_IDLE;

SamplingResult lastSamplingResult = {
    false,
    "",
    0,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0,
    ""
};

static uint32_t lastTick = 0;
static uint32_t elapsedSecond = 0;
static uint32_t targetSecond =
    24UL * 60UL * 60UL;


/* ------------------------------------------------------------------------ */
/*  A1 actuator control                                                     */
/* ------------------------------------------------------------------------ */

static bool writeA1(bool on)
{
    char cmd[96];

    snprintf(
        cmd,
        sizeof(cmd),
        "{\"cmd\":\"write_gpio\",\"pin\":\"A1\",\"val\":%d}",
        on ? 1 : 0
    );

    String response = sendCommand(String(cmd));

    if (response.length() == 0)
    {
        Serial.println("A1 ERROR: no response");
        return false;
    }

    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, response);

    if (err)
    {
        Serial.print("A1 JSON ERROR: ");
        Serial.println(response);
        return false;
    }

    bool ok = doc["status"] == "ok";

    Serial.print("A1 ");
    Serial.print(on ? "ON" : "OFF");
    Serial.print(" -> ");
    Serial.println(ok ? "OK" : "ERROR");

    return ok;
}


/* ------------------------------------------------------------------------ */
/*  Capture sampling result                                                 */
/* ------------------------------------------------------------------------ */

static void captureSamplingResult()
{
    lastSamplingResult.valid = true;

    lastSamplingResult.datetime =
        systemInfo.datetime;

    lastSamplingResult.duration_seconds =
        elapsedSecond;

    lastSamplingResult.temperature =
        systemInfo.temperature;

    lastSamplingResult.humidity =
        systemInfo.humidity;

    lastSamplingResult.pressure =
        systemInfo.pressure;

    lastSamplingResult.ac_voltage =
        systemInfo.ac_voltage;

    /*
     * Wind sensor has not been integrated yet.
     * Keep these values explicitly marked as unavailable.
     */
    lastSamplingResult.wind_speed = 0.0f;
    lastSamplingResult.wind_direction = 0;
    lastSamplingResult.wind_cardinal = "N/A";


    Serial.println();
    Serial.println("==============================");
    Serial.println("[SAMPLING] RESULT SNAPSHOT");
    Serial.println("==============================");

    Serial.print("[RESULT] datetime    : ");
    Serial.println(lastSamplingResult.datetime);

    Serial.print("[RESULT] duration    : ");
    Serial.println(lastSamplingResult.duration_seconds);

    Serial.print("[RESULT] temperature : ");
    Serial.println(lastSamplingResult.temperature);

    Serial.print("[RESULT] humidity    : ");
    Serial.println(lastSamplingResult.humidity);

    Serial.print("[RESULT] pressure    : ");
    Serial.println(lastSamplingResult.pressure);

    Serial.print("[RESULT] AC voltage  : ");
    Serial.println(lastSamplingResult.ac_voltage);

    Serial.println("[RESULT] wind        : N/A");

    Serial.println("==============================");
    Serial.println();
}


/* ------------------------------------------------------------------------ */
/*  Initialization                                                          */
/* ------------------------------------------------------------------------ */

void sampling_init()
{
    samplingState = SAMPLING_IDLE;

    elapsedSecond = 0;

    lastTick = millis();

    lastSamplingResult.valid = false;

    /*
     * Safe state on boot:
     * make sure A1 is OFF.
     */
    writeA1(false);
}


/* ------------------------------------------------------------------------ */
/*  START                                                                    */
/* ------------------------------------------------------------------------ */

void sampling_start()
{
    if (samplingState == SAMPLING_RUNNING)
        return;


    /*
     * START from PAUSED:
     * resume the existing timer.
     */
    if (samplingState == SAMPLING_PAUSED)
    {
        if (!writeA1(true))
            return;

        lastTick = millis();

        samplingState = SAMPLING_RUNNING;

        Serial.println(
            "SAMPLING -> RUNNING (resume)"
        );

        return;
    }


    /*
     * START from IDLE or FINISHED:
     * begin a completely new sampling run.
     */
    if (!writeA1(true))
        return;

    elapsedSecond = 0;

    lastTick = millis();

    lastSamplingResult.valid = false;

    samplingState = SAMPLING_RUNNING;

    Serial.println(
        "SAMPLING -> RUNNING (new run)"
    );
}


/* ------------------------------------------------------------------------ */
/*  PAUSE                                                                    */
/* ------------------------------------------------------------------------ */

void sampling_pause()
{
    if (samplingState != SAMPLING_RUNNING)
        return;


    if (!writeA1(false))
        return;


    samplingState = SAMPLING_PAUSED;

    Serial.println(
        "SAMPLING -> PAUSED (A1 OFF)"
    );
}


/* ------------------------------------------------------------------------ */
/*  STOP                                                                     */
/* ------------------------------------------------------------------------ */

void sampling_stop()
{
    /*
     * STOP must always force the actuator OFF.
     */
    if (!writeA1(false))
        return;


    /*
     * Only capture a result if an actual sampling run existed.
     */
    if (samplingState == SAMPLING_RUNNING ||
        samplingState == SAMPLING_PAUSED)
    {
        /*
         * IMPORTANT:
         * Capture BEFORE resetting elapsedSecond.
         */
        captureSamplingResult();
    }


    samplingState = SAMPLING_IDLE;

    elapsedSecond = 0;

    lastTick = millis();

    Serial.println(
        "SAMPLING -> STOPPED (A1 OFF)"
    );
}


/* ------------------------------------------------------------------------ */
/*  Sampling timer task                                                      */
/* ------------------------------------------------------------------------ */

void sampling_task()
{
    if (samplingState != SAMPLING_RUNNING)
        return;


    uint32_t now = millis();

    if (now - lastTick < 1000)
        return;


    /*
     * Preserve elapsed time even if loop()
     * is delayed for a little while.
     */
    uint32_t elapsedTicks =
        (now - lastTick) / 1000;

    lastTick += elapsedTicks * 1000;

    elapsedSecond += elapsedTicks;


    /*
     * Automatic finish after 24 hours.
     */
    if (elapsedSecond >= targetSecond)
    {
        elapsedSecond = targetSecond;


        if (writeA1(false))
        {
            /*
             * Capture the final result before
             * changing the state.
             */
            captureSamplingResult();

            samplingState = SAMPLING_FINISHED;

            Serial.println(
                "SAMPLING -> FINISHED (A1 OFF)"
            );
        }
    }
}


/* ------------------------------------------------------------------------ */
/*  Timer display                                                             */
/* ------------------------------------------------------------------------ */

String getElapsedTime()
{
    char buf[16];

    uint32_t h =
        elapsedSecond / 3600;

    uint32_t m =
        (elapsedSecond % 3600) / 60;

    uint32_t s =
        elapsedSecond % 60;


    snprintf(
        buf,
        sizeof(buf),
        "%02lu:%02lu:%02lu",
        (unsigned long)h,
        (unsigned long)m,
        (unsigned long)s
    );

    return String(buf);
}


String getRemainingTime()
{
    uint32_t remain =
        (elapsedSecond >= targetSecond)
        ? 0
        : (targetSecond - elapsedSecond);


    char buf[16];

    uint32_t h =
        remain / 3600;

    uint32_t m =
        (remain % 3600) / 60;

    uint32_t s =
        remain % 60;


    snprintf(
        buf,
        sizeof(buf),
        "%02lu:%02lu:%02lu",
        (unsigned long)h,
        (unsigned long)m,
        (unsigned long)s
    );

    return String(buf);
}


/* ------------------------------------------------------------------------ */
/*  State                                                                     */
/* ------------------------------------------------------------------------ */

SamplingState getSamplingState()
{
    return samplingState;
}

bool sampling_set_duration(uint32_t seconds)
{
    /*
     * Duration should only be changed
     * when sampling is not running.
     */
    if (samplingState == SAMPLING_RUNNING ||
        samplingState == SAMPLING_PAUSED)
    {
        Serial.println(
            "[SAMPLING] Cannot change duration while active"
        );

        return false;
    }

    if (seconds == 0)
    {
        Serial.println(
            "[SAMPLING] Invalid duration: 0 seconds"
        );

        return false;
    }

    targetSecond = seconds;

    Serial.print(
        "[SAMPLING] Duration set to "
    );

    Serial.print(targetSecond);

    Serial.println(" seconds");

    return true;
}


uint32_t sampling_get_duration()
{
    return targetSecond;
}

uint32_t sampling_get_elapsed_seconds()
{
    return elapsedSecond;
}

/* ------------------------------------------------------------------------ */
/*  Last result                                                              */
/* ------------------------------------------------------------------------ */

const SamplingResult &getLastSamplingResult()
{
    return lastSamplingResult;
}
