#include "printer_controller.h"
#include "communication.h"
#include "sampling_controller.h"

static bool printerReady = false;


bool printer_is_ready()
{
    return printerReady;
}


bool printer_init()
{
    printerReady = false;

    Serial.println("[PRINTER] INIT");

    return printer_check();
}


bool printer_check()
{
    Serial.println("[PRINTER] CHECK");

    String response = sendCommand(
        "{\"cmd\":\"get_printer\"}"
    );

    if (response.length() == 0)
    {
        Serial.println("[PRINTER] NO RESPONSE");
        printerReady = false;
        return false;
    }

    Serial.print("[PRINTER] RX -> ");
    Serial.println(response);

    if (response.indexOf("\"status\":\"ok\"") >= 0 &&
        response.indexOf("\"connected\":true") >= 0)
    {
        printerReady = true;
        Serial.println("[PRINTER] READY");
        return true;
    }

    printerReady = false;
    Serial.println("[PRINTER] NOT READY");

    return false;
}


bool printer_print_line(const char *text)
{
    if (!printerReady)
    {
        Serial.println("[PRINTER] NOT READY");
        return false;
    }

    String json =
        "{\"cmd\":\"print_text\",\"text\":\"";

    for (const char *p = text; *p; ++p)
    {
        if (*p == '\\')
            json += "\\\\";
        else if (*p == '"')
            json += "\\\"";
        else
            json += *p;
    }

    json += "\\n\"}";

    String response = sendCommand(json);

    Serial.print("[PRINTER] PRINT RX -> ");
    Serial.println(response);

    if (response.indexOf("\"status\":\"ok\"") >= 0 &&
        response.indexOf("\"msg\":\"printed\"") >= 0)
    {
        Serial.println("[PRINTER] LINE PRINTED");
        return true;
    }

    Serial.println("[PRINTER] PRINT ERROR");

    return false;
}


bool printer_print_result()
{
    if (!printerReady)
    {
        Serial.println("[PRINTER] NOT READY");
        return false;
    }

    const SamplingResult &result =
        getLastSamplingResult();

    if (!result.valid)
    {
        Serial.println("[PRINTER] NO VALID RESULT");
        return false;
    }

    Serial.println("==============================");
    Serial.println("[PRINTER] PRINT RESULT START");
    Serial.println("==============================");

    char line[128];

    if (!printer_print_line("=== HVAS RESULT ==="))
        return false;


    snprintf(
        line,
        sizeof(line),
        "Date/Time: %s",
        result.datetime.c_str()
    );

    if (!printer_print_line(line))
        return false;


    uint32_t h =
        result.duration_seconds / 3600;

    uint32_t m =
        (result.duration_seconds % 3600) / 60;

    uint32_t s =
        result.duration_seconds % 60;

    snprintf(
        line,
        sizeof(line),
        "Duration: %02lu:%02lu:%02lu",
        (unsigned long)h,
        (unsigned long)m,
        (unsigned long)s
    );

    if (!printer_print_line(line))
        return false;


    snprintf(
        line,
        sizeof(line),
        "Temperature: %.2f C",
        result.temperature
    );

    if (!printer_print_line(line))
        return false;


    snprintf(
        line,
        sizeof(line),
        "Humidity: %.2f %%RH",
        result.humidity
    );

    if (!printer_print_line(line))
        return false;


    snprintf(
        line,
        sizeof(line),
        "Pressure: %.2f hPa",
        result.pressure
    );

    if (!printer_print_line(line))
        return false;


    snprintf(
        line,
        sizeof(line),
        "AC Voltage: %.2f V",
        result.ac_voltage
    );

    if (!printer_print_line(line))
        return false;


    if (result.wind_cardinal == "N/A")
    {
        if (!printer_print_line("Wind: N/A"))
            return false;
    }
    else
    {
        snprintf(
            line,
            sizeof(line),
            "Wind: %.2f m/s %d deg %s",
            result.wind_speed,
            result.wind_direction,
            result.wind_cardinal.c_str()
        );

        if (!printer_print_line(line))
            return false;
    }


    if (!printer_print_line("================"))
        return false;


    printer_feed(3);

    Serial.println("[PRINTER] PRINT RESULT DONE");

    return true;
}


void printer_feed(uint8_t lines)
{
    if (!printerReady)
        return;

    char json[96];

    snprintf(
        json,
        sizeof(json),
        "{\"cmd\":\"print_feed\",\"lines\":%u}",
        lines
    );

    String response = sendCommand(json);

    Serial.print("[PRINTER] FEED RX -> ");
    Serial.println(response);
}

