#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#define RS485_TX 16
#define RS485_RX 15

struct SystemInfo
{
    // ===== System =====
    String status;
    String device;
    String version;

    uint32_t uptime_ms = 0;
    uint32_t power_on_count = 0;

    // ===== RTC =====
    bool rtc_connected = false;
    String datetime;
    float rtc_temperature = 0.0f;

    // ===== BME280 =====
    bool bme_connected = false;
    float temperature = 0.0f;
    float humidity = 0.0f;
    float pressure = 0.0f;

    // ===== Voltage =====
    bool voltage_connected = false;
    float dc_voltage = 0.0f;

    // ===== GPS =====
    bool gps_connected = false;
    bool gps_fix = false;
    double latitude = 0.0;
    double longitude = 0.0;
    float altitude = 0.0f;
    float speed_kmh = 0.0f;
    uint8_t satellites = 0;

    // ===== PZEM =====
    bool pzem_connected = false;
    float ac_voltage = 0.0f;
    float current = 0.0f;
    float power = 0.0f;
    float energy = 0.0f;
    float frequency = 0.0f;
    float power_factor = 0.0f;
};

extern SystemInfo systemInfo;

void comm_init();
String sendCommand(String cmd);
void comm_task();

bool parseSystem(String json);
bool parseRTC(String json);
bool parseBME(String json);
bool parseVoltage(String json);
bool parseGPS(String json);
bool parsePZEM(String json);

bool comm_refresh_bme();
bool comm_refresh_gps();
bool comm_refresh_rtc();
