#include "ble_manager.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>
#include <math.h>

#include "communication.h"
#include "home_controller.h"
#include "sampling_controller.h"
#include "result_controller.h"

#include <ArduinoJson.h>

/*
 * BLE identity for the HVAS product.
 * This is the name that should appear in a BLE scanner.
 */
static const char *BLE_DEVICE_NAME = "ENVILIFE TSP HVAS";

/*
 * Keep these UUIDs fixed once the mobile application starts using them.
 * They are HVAS-specific UUIDs and are independent from the UART protocol.
 */
static const char *HVAS_SERVICE_UUID =
    "7f4a1000-5d6b-4e7f-9a10-000000000001";

static const char *HVAS_COMMAND_UUID =
    "7f4a1000-5d6b-4e7f-9a10-000000000002";

static const char *HVAS_RESPONSE_UUID =
    "7f4a1000-5d6b-4e7f-9a10-000000000003";

static BLEServer *s_server = nullptr;
static BLECharacteristic *s_command = nullptr;
static BLECharacteristic *s_response = nullptr;
static bool s_connected = false;

// =========================================================
// BLE REAL-TIME TELEMETRY STREAM
// =========================================================
static bool s_stream_enabled = false;
static uint32_t s_stream_interval_ms = 1000;
static uint32_t s_last_stream_ms = 0;

static float round2(float value)
{
    return roundf(value * 100.0f) / 100.0f;
}

/*
 * A small queue keeps BLE callbacks lightweight.
 * The actual UART transaction is performed from ble_task(), not inside
 * the Bluetooth callback.
 */
struct BleCommandPacket
{
    char data[512];
};

static QueueHandle_t s_command_queue = nullptr;

class HVASServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *server) override
    {
        s_connected = true;

        Serial.println();
        Serial.println("[BLE] CLIENT CONNECTED");
        Serial.printf("[BLE] Device : %s\n", BLE_DEVICE_NAME);
    }

    void onDisconnect(BLEServer *server) override
    {
        s_connected = false;
        s_stream_enabled = false;

        Serial.println();
        Serial.println("[BLE] CLIENT DISCONNECTED");

        /*
         * Start advertising again so another phone can reconnect.
         */
        delay(20);
        server->getAdvertising()->start();

        Serial.println("[BLE] ADVERTISING AGAIN");
    }
};

class HVASCommandCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *characteristic) override
    {
        String value = characteristic->getValue();

        if (value.length() == 0)
            return;

        if (value.length() >= sizeof(BleCommandPacket))
        {
            Serial.printf("[BLE] COMMAND TOO LONG (%u bytes)\n",
                          (unsigned)value.length());

            if (s_response)
            {
                s_response->setValue(
                    "{\"status\":\"error\",\"err\":\"BLE_CMD_TOO_LONG\"}");

                if (s_connected)
                    s_response->notify();
            }

            return;
        }

        if (!s_command_queue)
        {
            Serial.println("[BLE] ERROR: command queue not ready");
            return;
        }

        BleCommandPacket packet{};

        memcpy(packet.data, value.c_str(), value.length());
        packet.data[value.length()] = '\0';

        if (xQueueSend(s_command_queue, &packet, 0) != pdTRUE)
        {
            Serial.println("[BLE] COMMAND QUEUE FULL");

            if (s_response)
            {
                s_response->setValue(
                    "{\"status\":\"error\",\"err\":\"BLE_QUEUE_FULL\"}");

                if (s_connected)
                    s_response->notify();
            }

            return;
        }

        Serial.print("[BLE] RX <- ");
        Serial.println(packet.data);
    }
};

void ble_init()
{
    if (s_command_queue == nullptr)
    {
        s_command_queue = xQueueCreate(4, sizeof(BleCommandPacket));
    }

    if (s_command_queue == nullptr)
    {
        Serial.println("[BLE] ERROR: queue creation failed");
        return;
    }

    Serial.println();
    Serial.println("==========================");
    Serial.println("BLE INITIALIZING");
    Serial.println("==========================");

    BLEDevice::init(BLE_DEVICE_NAME);

    s_server = BLEDevice::createServer();
    s_server->setCallbacks(new HVASServerCallbacks());

    BLEService *service =
        s_server->createService(HVAS_SERVICE_UUID);

    /*
     * Mobile -> ESP32
     */
    s_command = service->createCharacteristic(
        HVAS_COMMAND_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR);

    s_command->setCallbacks(new HVASCommandCallbacks());

    /*
     * ESP32 -> Mobile
     */
    s_response = service->createCharacteristic(
        HVAS_RESPONSE_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY);

    s_response->addDescriptor(new BLE2902());
    s_response->setValue("{\"status\":\"ok\",\"msg\":\"BLE_READY\"}");

    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(HVAS_SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);

    BLEDevice::startAdvertising();

    Serial.println("[BLE] READY");
    Serial.printf("[BLE] NAME    : %s\n", BLE_DEVICE_NAME);
    Serial.printf("[BLE] SERVICE : %s\n", HVAS_SERVICE_UUID);
    Serial.printf("[BLE] COMMAND : %s (WRITE)\n", HVAS_COMMAND_UUID);
    Serial.printf("[BLE] RESPONSE: %s (NOTIFY)\n", HVAS_RESPONSE_UUID);
    Serial.println("[BLE] Advertising started");
}

static const char *samplingStateToString()
{
    switch (getSamplingState())
    {
        case SAMPLING_IDLE:
            return "stopped";

        case SAMPLING_RUNNING:
            return "running";

        case SAMPLING_PAUSED:
            return "paused";

        case SAMPLING_FINISHED:
            return "finished";

        default:
            return "unknown";
    }
}

static bool ble_notify_json(const String &payload, const char *label)
{
    if (!s_connected || !s_response)
        return false;

    // BLE integration contract: keep each application-level notification
    // below 511 bytes. The previous full telemetry payload was ~653 bytes.
    if (payload.length() > 511)
    {
        Serial.printf("[BLE] %s TOO LARGE: %u bytes\n",
                      label,
                      (unsigned)payload.length());
        return false;
    }

    s_response->setValue(payload.c_str());
    s_response->notify();

    Serial.printf("[BLE] %s -> %u bytes: ",
                  label,
                  (unsigned)payload.length());
    Serial.println(payload);

    return true;
}

static bool ble_send_telemetry()
{
    if (!s_connected || !s_response)
        return false;

    JsonDocument doc;
    doc["type"] = "telemetry";

    // Sampling Status & Timers
    doc["state"] = samplingStateToString();
    doc["elapsed"] = getElapsedTime();
    doc["remain"] = getRemainingTime();

    // Live Sensor Readings (Direct & Compact)
    doc["temp"] = systemInfo.temperature;
    doc["hum"] = systemInfo.humidity;
    doc["press"] = systemInfo.pressure;
    doc["flow"] = home_get_flow_setpoint();

    // Nested BME for structured compatibility
    doc["bme"]["temp"] = systemInfo.temperature;
    doc["bme"]["hum"] = systemInfo.humidity;
    doc["bme"]["press"] = systemInfo.pressure;

    // RTC & Voltage
    doc["rtc"] = systemInfo.datetime;
    doc["volt"] = systemInfo.dc_voltage;

    String payload;
    serializeJson(doc, payload);

    Serial.printf(
        "[BLE] TELEMETRY SIZE = %u bytes\n",
        (unsigned)payload.length()
    );

    return ble_notify_json(
        payload,
        "TELEMETRY"
    );
}

static bool ble_send_system_info()
{
    if (!s_connected || !s_response)
        return false;

    JsonDocument doc;
    doc["type"] = "system";
    doc["status"] = systemInfo.status;
    doc["device"] = systemInfo.device;
    doc["version"] = systemInfo.version;
    doc["uptime_ms"] = systemInfo.uptime_ms;
    doc["power_on_count"] = systemInfo.power_on_count;

    String payload;
    serializeJson(doc, payload);
    return ble_notify_json(payload, "SYSTEM");
}

static bool handleLocalCommand(
    const String &command,
    String &response
)
{
    JsonDocument doc;

    DeserializationError err =
        deserializeJson(doc, command);

    if (err)
    {
        return false;
    }

    const char *cmd = doc["cmd"];

    if (!cmd)
    {
        return false;
    }

    // =========================================================
    // START SAMPLING
    // =========================================================
    if (strcmp(cmd, "start_sampling") == 0)
    {
        /*
        * Optional sampling duration.
        *
        * Example:
        * {
        *   "cmd":"start_sampling",
        *   "duration_minutes":10,
        *   "stream_interval":1000
        * }
        *
        * If duration_minutes is omitted,
        * keep the currently configured LCD duration.
        */
        if (doc["duration_minutes"].is<uint32_t>())
        {
            uint32_t minutes = doc["duration_minutes"];

            if (minutes == 0)
            {
                response =
                    "{\"status\":\"error\","
                    "\"cmd\":\"start_sampling\","
                    "\"err\":\"INVALID_DURATION\"}";

                return true;
            }

            uint64_t seconds64 =
                (uint64_t)minutes * 60ULL;

            if (seconds64 > 0xFFFFFFFFULL)
            {
                response =
                    "{\"status\":\"error\","
                    "\"cmd\":\"start_sampling\","
                    "\"err\":\"DURATION_TOO_LARGE\"}";

                return true;
            }

            if (!home_set_sampling_duration((uint32_t)seconds64))
            {
                response =
                    "{\"status\":\"error\","
                    "\"cmd\":\"start_sampling\","
                    "\"err\":\"DURATION_SET_FAILED\"}";

                return true;
            }
        }

        /*
        * Optional BLE telemetry interval.
        * Default = 1000 ms.
        */
        uint32_t interval = doc["stream_interval"] | 1000;

        if (interval < 250)
            interval = 250;

        if (interval > 60000)
            interval = 60000;

        /*
        * Start sampling.
        */
        home_start_sampling();

        if (getSamplingState() != SAMPLING_RUNNING)
        {
            response =
                "{\"status\":\"error\","
                "\"cmd\":\"start_sampling\","
                "\"err\":\"START_FAILED\"}";

            return true;
        }

        /*
        * Immediately refresh BME + GPS.
        * Do not wait for comm_task() next 1-second cycle.
        */
        comm_refresh_bme();
        comm_refresh_gps();

        /*
        * Enable realtime BLE telemetry.
        */
        s_stream_enabled = true;
        s_stream_interval_ms = interval;
        s_last_stream_ms = millis();

        uint32_t duration =
            home_get_sampling_duration();

        response =
            "{\"status\":\"ok\","
            "\"cmd\":\"start_sampling\","
            "\"stream\":true,"
            "\"stream_interval\":" +
            String(s_stream_interval_ms) +
            ","
            "\"duration_seconds\":" +
            String(duration) +
            "}";

        Serial.println("[BLE] START SAMPLING");

        Serial.print("[BLE] Duration : ");
        Serial.print(duration);
        Serial.println(" sec");

        Serial.print("[BLE] Stream interval : ");
        Serial.print(s_stream_interval_ms);
        Serial.println(" ms");

        return true;
    }

    // =========================================================
    // PAUSE SAMPLING
    // =========================================================
    if (strcmp(cmd, "pause_sampling") == 0)
    {
        home_pause_sampling();

        const char *state = samplingStateToString();

        response =
            "{\"status\":\"ok\","
            "\"cmd\":\"pause_sampling\","
            "\"state\":\"" +
            String(state) +
            "\"}";

        Serial.println("[BLE] LOCAL -> PAUSE SAMPLING");

        return true;
    }

    // =========================================================
    // STOP SAMPLING
    // =========================================================
    if (strcmp(cmd, "stop_sampling") == 0)
    {
        home_stop_sampling();

        // Sampling has stopped, so realtime BLE telemetry
        // must also stop.
        s_stream_enabled = false;

        const char *state = samplingStateToString();

        response =
            "{\"status\":\"ok\","
            "\"cmd\":\"stop_sampling\","
            "\"state\":\"" +
            String(state) +
            "\"}";

        Serial.println("[BLE] LOCAL -> STOP SAMPLING");
        Serial.println("[BLE] STREAM AUTO-STOPPED");

        return true;
    }

    // =========================================================
    // START SAMPLING + REAL-TIME TELEMETRY
    // =========================================================
    if (strcmp(cmd, "start_stream") == 0)
    {
        /*
        * Optional sampling duration.
        *
        * Example:
        * {
        *   "cmd":"start_stream",
        *   "duration_minutes":10,
        *   "interval":1000
        * }
        */
        if (doc["duration_minutes"].is<uint32_t>())
        {
            uint32_t minutes = doc["duration_minutes"];

            if (minutes == 0)
            {
                response =
                    "{\"status\":\"error\","
                    "\"cmd\":\"start_stream\","
                    "\"err\":\"INVALID_DURATION\"}";

                return true;
            }

            uint64_t seconds64 =
                (uint64_t)minutes * 60ULL;

            if (seconds64 > 0xFFFFFFFFULL)
            {
                response =
                    "{\"status\":\"error\","
                    "\"cmd\":\"start_stream\","
                    "\"err\":\"DURATION_TOO_LARGE\"}";

                return true;
            }

            if (!home_set_sampling_duration((uint32_t)seconds64))
            {
                response =
                    "{\"status\":\"error\","
                    "\"cmd\":\"start_stream\","
                    "\"err\":\"DURATION_SET_FAILED\"}";

                return true;
            }
        }

        /*
        * BLE telemetry interval.
        */
        uint32_t interval = doc["interval"] | 1000;

        if (interval < 250)
            interval = 250;

        if (interval > 60000)
            interval = 60000;

        /*
        * Start sampling if necessary.
        */
        SamplingState currentState =
            getSamplingState();

        if (currentState != SAMPLING_RUNNING)
        {
            home_start_sampling();
        }

        if (getSamplingState() != SAMPLING_RUNNING)
        {
            response =
                "{\"status\":\"error\","
                "\"cmd\":\"start_stream\","
                "\"err\":\"START_FAILED\"}";

            return true;
        }

        /*
        * Immediately refresh BME + GPS.
        */
        comm_refresh_bme();
        comm_refresh_gps();

        /*
        * Enable telemetry.
        */
        s_stream_interval_ms = interval;
        s_stream_enabled = true;
        s_last_stream_ms = millis();

        uint32_t duration =
            home_get_sampling_duration();

        const char *state =
            samplingStateToString();

        response =
            "{\"status\":\"ok\","
            "\"cmd\":\"start_stream\","
            "\"state\":\"" +
            String(state) +
            "\","
            "\"interval\":" +
            String(s_stream_interval_ms) +
            ","
            "\"duration_seconds\":" +
            String(duration) +
            "}";

        Serial.print("[BLE] START SAMPLING + STREAM interval=");
        Serial.println(s_stream_interval_ms);

        Serial.print("[BLE] Duration=");
        Serial.print(duration);
        Serial.println(" sec");

        return true;
    }

    // =========================================================
    // STOP REAL-TIME TELEMETRY STREAM
    // =========================================================
    if (strcmp(cmd, "stop_stream") == 0)
    {
        s_stream_enabled = false;

        response =
            "{\"status\":\"ok\","
            "\"cmd\":\"stop_stream\"}";

        Serial.println("[BLE] LOCAL -> STOP STREAM");

        return true;
    }

    // =========================================================
    // PRINT ONLY
    // =========================================================
    if (strcmp(cmd, "print_only") == 0)
    {
        bool ok = result_print();

        if (ok)
        {
            response =
                "{\"status\":\"ok\","
                "\"cmd\":\"print_only\"}";
        }
        else
        {
            response =
                "{\"status\":\"error\","
                "\"cmd\":\"print_only\","
                "\"err\":\"PRINT_FAILED\"}";
        }

        Serial.println("[BLE] LOCAL -> PRINT ONLY");

        return true;
    }

    // =========================================================
    // SAVE ONLY
    // =========================================================
    if (strcmp(cmd, "save_only") == 0)
    {
        bool ok = result_save();

        if (ok)
        {
            response =
                "{\"status\":\"ok\","
                "\"cmd\":\"save_only\"}";
        }
        else
        {
            response =
                "{\"status\":\"error\","
                "\"cmd\":\"save_only\","
                "\"err\":\"SAVE_FAILED\"}";
        }

        Serial.println("[BLE] LOCAL -> SAVE ONLY");

        return true;
    }

    // =========================================================
    // PRINT & SAVE
    // =========================================================
    if (strcmp(cmd, "print_and_save") == 0)
    {
        bool ok = result_print_and_save();

        if (ok)
        {
            response =
                "{\"status\":\"ok\","
                "\"cmd\":\"print_and_save\"}";
        }
        else
        {
            response =
                "{\"status\":\"error\","
                "\"cmd\":\"print_and_save\","
                "\"err\":\"PRINT_OR_SAVE_FAILED\"}";
        }

        Serial.println("[BLE] LOCAL -> PRINT & SAVE");

        return true;
    }

    return false;
}

void ble_task()
{
    if (!s_command_queue || !s_response)
        return;

    // =========================================================
    // 1. PROCESS BLE COMMAND
    // =========================================================
    BleCommandPacket packet{};

    if (xQueueReceive(s_command_queue, &packet, 0) == pdTRUE)
    {
        if (s_connected)
        {
            String command(packet.data);
            command.trim();

            if (command.length() > 0)
            {
                String response;

                if (handleLocalCommand(command, response))
                {
                    Serial.print("[BLE] LOCAL TX -> ");
                    Serial.println(response);
                }
                else
                {
                    response = sendCommand(command);

                    if (response.length() == 0)
                    {
                        response =
                            "{\"status\":\"error\","
                            "\"err\":\"ATMEGA_NO_RESPONSE\"}";
                    }

                    Serial.print("[BLE] ATMEGA TX -> ");
                    Serial.println(response);
                }

                // ACK first; telemetry is a separate notification.
                ble_notify_json(response, "RESPONSE");

                JsonDocument commandDoc;
                if (deserializeJson(commandDoc, command) == DeserializationError::Ok)
                {
                    const char *cmd = commandDoc["cmd"] | "";

                    if (strcmp(cmd, "start_sampling") == 0 ||
                        strcmp(cmd, "start_stream") == 0 ||
                        strcmp(cmd, "pause_sampling") == 0 ||
                        strcmp(cmd, "stop_sampling") == 0)
                    {
                        // Immediate snapshot so the mobile UI mirrors the
                        // command without waiting for the next interval.
                        ble_send_telemetry();
                    }
                }
            }
        }
    }

    // =========================================================
    // 2. SYSTEM METADATA (small packet, sent every 5 seconds)
    // =========================================================
    static uint32_t lastSystemBle = 0;
    if (s_connected && (millis() - lastSystemBle >= 5000))
    {
        lastSystemBle = millis();
        ble_send_system_info();
    }

    // =========================================================
    // 3. REAL-TIME TELEMETRY STREAM
    // =========================================================
    if (s_connected && s_stream_enabled)
    {
        // Automatic 24-hour completion: send one final packet with
        // state=finished, then stop the stream.
        if (getSamplingState() == SAMPLING_FINISHED)
        {
            ble_send_telemetry();
            ble_send_system_info();
            s_stream_enabled = false;
            Serial.println("[BLE] STREAM AUTO-STOPPED (SAMPLING FINISHED)");
        }
        else
        {
            uint32_t now = millis();

            if (now - s_last_stream_ms >= s_stream_interval_ms)
            {
                s_last_stream_ms = now;
                ble_send_telemetry();
            }
        }
    }
}

bool ble_is_connected()
{
    return s_connected;
}
