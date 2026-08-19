#include "communication.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

HardwareSerial RS485(2);
SystemInfo systemInfo;

static SemaphoreHandle_t rs485Mutex = nullptr;

void comm_init()
{
    rs485Mutex = xSemaphoreCreateMutex();

    if (rs485Mutex == nullptr)
    {
        Serial.println("[RS485] ERROR: mutex creation failed");
        return;
    }

    RS485.begin(
        115200,
        SERIAL_8N1,
        RS485_RX,
        RS485_TX);

    Serial.println();
    Serial.println("==========================");
    Serial.println("RS485 READY");
    Serial.println("==========================");
}

String sendCommand(String cmd)
{
    String response = "";

    if (rs485Mutex == nullptr)
    {
        Serial.println("[RS485] ERROR: mutex not initialized");
        return response;
    }

    /*
     * Only one task is allowed to use RS485
     * at a time.
     */
    if (xSemaphoreTake(rs485Mutex, pdMS_TO_TICKS(1500)) != pdTRUE)
    {
        Serial.println("[RS485] ERROR: mutex timeout");
        return response;
    }


    /*
     * Clear stale RX data before sending
     * a new command.
     */
    while (RS485.available())
    {
        RS485.read();
    }


    /*
     * Send command.
     */
    RS485.println(cmd);

    Serial.print("TX -> ");
    Serial.println(cmd);


    /*
     * Wait for exactly one line of response.
     */
    unsigned long start = millis();

    while (millis() - start < 1000)
    {
        while (RS485.available())
        {
            char c = RS485.read();

            if (c == '\n')
            {
                xSemaphoreGive(rs485Mutex);

                return response;
            }

            if (c != '\r')
            {
                response += c;
            }
        }

        /*
         * Give the FreeRTOS scheduler a chance
         * while waiting for the response.
         */
        vTaskDelay(pdMS_TO_TICKS(1));
    }


    /*
     * Timeout.
     */
    Serial.print("[RS485] TIMEOUT: ");
    Serial.println(cmd);

    xSemaphoreGive(rs485Mutex);

    return response;
}

void comm_task()
{
    static uint32_t lastSystem = 0;
    static uint32_t lastRTC = 0;
    static uint32_t lastBME = 0;
    static uint32_t lastVolt = 0;
    static uint32_t lastGPS = 0;
    static uint32_t lastPZEM = 0;

    String res;

    // =========================
    // SYSTEM
    // =========================
    if (millis() - lastSystem >= 5000)
    {
        lastSystem = millis();

        res = sendCommand("{\"cmd\":\"get_system\"}");

        if (parseSystem(res))
        {
            Serial.println("[SYSTEM] OK");
        }
    }

    // =========================
    // RTC
    // =========================
    if (millis() - lastRTC >= 1000)
    {
        lastRTC = millis();

        res = sendCommand("{\"cmd\":\"get_rtc\"}");

        if (parseRTC(res))
        {
            Serial.println("[RTC] OK");
        }
    }

    // =========================
    // BME280
    // =========================
    if (millis() - lastBME >= 1000)
    {
        lastBME = millis();

        res = sendCommand("{\"cmd\":\"get_bme\"}");

        if (parseBME(res))
        {
            Serial.println("[BME] OK");
        }
    }

    // =========================
    // VOLTAGE
    // =========================
    if (millis() - lastVolt >= 1000)
    {
        lastVolt = millis();

        res = sendCommand("{\"cmd\":\"get_voltage\"}");

        if (parseVoltage(res))
        {
            Serial.println("[VOLT] OK");
        }
    }

    // =========================
    // GPS
    // =========================
    if (millis() - lastGPS >= 1000)
    {
        lastGPS = millis();

        res = sendCommand("{\"cmd\":\"get_gps\"}");

        if (parseGPS(res))
        {
            Serial.println("[GPS] OK");
        }
    }

    // =========================
    // PZEM
    // =========================
    if (millis() - lastPZEM >= 2000)
    {
        lastPZEM = millis();

        res = sendCommand("{\"cmd\":\"get_pzem\"}");

        parsePZEM(res);
    }
}

bool parseSystem(String json)
{
    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, json);

    if (err)
    {
        Serial.println("JSON ERROR");

        return false;
    }

    systemInfo.status = doc["status"].as<String>();

    systemInfo.device = doc["device"].as<String>();

    systemInfo.version = doc["version"].as<String>();

    systemInfo.uptime_ms = doc["uptime_ms"];

    systemInfo.power_on_count = doc["power_on_count"];

    return true;
}

bool parseRTC(String json)
{
    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, json);

    if (err)
    {
        Serial.println("RTC JSON ERROR");
        return false;
    }

    systemInfo.rtc_connected =
        doc["connected"];

    systemInfo.datetime =
        doc["datetime"].as<String>();

    systemInfo.rtc_temperature =
        doc["temp_rtc"];

    return true;
}

bool parseBME(String json)
{
    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, json);

    if (err)
    {
        Serial.println("BME JSON ERROR");
        return false;
    }

    systemInfo.bme_connected =
        doc["connected"];

    systemInfo.temperature =
        doc["temp"];

    systemInfo.humidity =
        doc["hum"];

    systemInfo.pressure =
        doc["press"];

    return true;
}

bool parseVoltage(String json)
{
    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, json);

    if (err)
    {
        Serial.println("Voltage JSON ERROR");
        return false;
    }

    systemInfo.voltage_connected =
        doc["connected"];

    systemInfo.dc_voltage =
        doc["voltage"];

    return true;
}

bool parseGPS(String json)
{
    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, json);

    if (err)
    {
        Serial.println("GPS JSON ERROR");
        return false;
    }

    systemInfo.gps_connected =
        doc["connected"];

    systemInfo.gps_fix =
        doc["fix"];

    systemInfo.latitude =
        doc["lat"];

    systemInfo.longitude =
        doc["lng"];

    systemInfo.altitude =
        doc["alt"];

    systemInfo.satellites =
        doc["sat"];

    systemInfo.speed_kmh =
        doc["speed_kmh"];

    return true;
}

bool parsePZEM(String json)
{
    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, json);

    if (err)
    {
        Serial.println("PZEM JSON ERROR");
        return false;
    }

    systemInfo.pzem_connected =
        doc["connected"];

    if (!systemInfo.pzem_connected)
        return false;

    systemInfo.ac_voltage =
        doc["voltage"];

    systemInfo.current =
        doc["current"];

    systemInfo.power =
        doc["power"];

    systemInfo.energy =
        doc["energy"];

    systemInfo.frequency =
        doc["frequency"];

    systemInfo.power_factor =
        doc["pf"];

    return true;
}

bool comm_refresh_bme()
{
    String res = sendCommand("{\"cmd\":\"get_bme\"}");

    if (res.length() == 0)
    {
        Serial.println("[BME] REFRESH FAILED: no response");
        return false;
    }

    if (parseBME(res))
    {
        Serial.println("[BME] REFRESH OK");
        return true;
    }

    Serial.println("[BME] REFRESH FAILED: parse error");
    return false;
}


bool comm_refresh_gps()
{
    String res = sendCommand("{\"cmd\":\"get_gps\"}");

    if (res.length() == 0)
    {
        Serial.println("[GPS] REFRESH FAILED: no response");
        return false;
    }

    if (parseGPS(res))
    {
        Serial.println("[GPS] REFRESH OK");
        return true;
    }

    Serial.println("[GPS] REFRESH FAILED: parse error");
    return false;
}


bool comm_refresh_rtc()
{
    String res = sendCommand("{\"cmd\":\"get_rtc\"}");

    if (res.length() == 0)
    {
        Serial.println("[RTC] REFRESH FAILED: no response");
        return false;
    }

    if (parseRTC(res))
    {
        Serial.println("[RTC] REFRESH OK");
        return true;
    }

    Serial.println("[RTC] REFRESH FAILED: parse error");
    return false;
}
