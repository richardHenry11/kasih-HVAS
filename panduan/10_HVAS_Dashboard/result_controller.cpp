#include "result_controller.h"

#include "sampling_controller.h"
#include "printer_controller.h"
#include "sd_controller.h"

#include <FS.h>
#include <SD.h>

void result_controller_init()
{
    Serial.println("[RESULT] CONTROLLER INIT");
}

bool result_print()
{
    const SamplingResult &result =
        getLastSamplingResult();

    if (!result.valid)
    {
        Serial.println(
            "[RESULT] NO VALID RESULT"
        );

        return false;
    }

    Serial.println(
        "[RESULT] PRINT REQUEST"
    );

    return printer_print_result();
}

bool result_save()
{
    Serial.println("[RESULT] SAVE REQUEST");

    // ---------------------------------------------------------
    // Check SD card
    // ---------------------------------------------------------
    if (!sd_controller_is_ready())
    {
        Serial.println("[RESULT] SD CARD NOT READY");
        return false;
    }

    // ---------------------------------------------------------
    // Get latest sampling result
    // ---------------------------------------------------------
    const SamplingResult &result =
        getLastSamplingResult();

    if (!result.valid)
    {
        Serial.println("[RESULT] NO VALID RESULT");
        return false;
    }

    // ---------------------------------------------------------
    // CSV file
    // ---------------------------------------------------------
    const char *filename = "/HVAS_RESULTS.csv";

    bool fileExists = SD.exists(filename);

    File file = SD.open(filename, FILE_APPEND);

    if (!file)
    {
        Serial.println("[RESULT] FAILED TO OPEN SD FILE");
        return false;
    }

    // ---------------------------------------------------------
    // Write CSV header only when file is newly created
    // ---------------------------------------------------------
    if (!fileExists)
    {
        file.println(
            "DateTime,Duration,Temperature,Humidity,Pressure,"
            "ACVoltage,WindSpeed,WindDirection,WindCardinal"
        );
    }

    // ---------------------------------------------------------
    // Write sampling result
    // ---------------------------------------------------------
    file.print("\"");
    file.print(result.datetime);
    file.print("\",");

    file.print(result.duration_seconds);
    file.print(",");

    file.print(result.temperature, 2);
    file.print(",");

    file.print(result.humidity, 2);
    file.print(",");

    file.print(result.pressure, 2);
    file.print(",");

    file.print(result.ac_voltage, 2);
    file.print(",");

    file.print(result.wind_speed, 2);
    file.print(",");

    file.print(result.wind_direction);
    file.print(",");

    file.print("\"");
    file.print(result.wind_cardinal);
    file.println("\"");

    file.close();

    Serial.println("[RESULT] SAVE OK");
    Serial.print("[RESULT] FILE: ");
    Serial.println(filename);

    return true;
}

bool result_print_and_save()
{
    bool printOK = result_print();

    bool saveOK = result_save();

    return printOK && saveOK;
}

bool result_execute(ResultAction action)
{
    switch (action)
    {
        case RESULT_ACTION_PRINT_ONLY:
            return result_print();

        case RESULT_ACTION_SAVE_ONLY:
            return result_save();

        case RESULT_ACTION_PRINT_AND_SAVE:
            return result_print_and_save();

        case RESULT_ACTION_NONE:
        default:
            return false;
    }
}
