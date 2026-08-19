#include <Arduino.h>
#include "dashboard.h"
#include "communication.h"
#include "home_controller.h"
#include "sampling_controller.h"
#include "result_controller.h"
#include "printer_controller.h"
#include "sd_controller.h"
#include "ble_manager.h"
#include <esp_display_panel.hpp>

#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "ui.h"
//#include <demos/lv_demos.h>

#include "esp_heap_caps.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

/**
 * To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 */
 // #include <demos/lv_demos.h>
 // #include <examples/lv_examples.h>

void setup()
{
    String title = "LVGL porting example";

    Serial.begin(115200);

    comm_init();

    /* Start BLE before the UI so the mobile app can discover the HVAS unit. */
    ble_init();

    printer_init();

    sampling_init();

    result_controller_init();

    home_init();

    dashboard_init();

    Serial.println("Initializing board");
    Board *board = new Board();
    board->init();

    #if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    /**
     * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
     * "bounce buffer" functionality to enhance the RGB data bandwidth.
     * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
     */
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif
    assert(board->begin());

    sd_controller_init((void *)board);

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Creating UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    lvgl_port_lock(-1);

    ui_init();

    /* Release the mutex */
    lvgl_port_unlock();
}

void loop()
{
    comm_task();
    ble_task();

    sampling_task();

    home_update();

    dashboard_update();

    static uint32_t lastHeap = 0;

    if (millis() - lastHeap > 1000)
    {
        lastHeap = millis();

        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t totalHeap = ESP.getHeapSize();

        uint32_t freePsram = ESP.getFreePsram();
        uint32_t totalPsram = ESP.getPsramSize();

        Serial.println("--------------------------------");

        Serial.printf("Heap : %u / %u (%.1f%% free)\n",
                      freeHeap,
                      totalHeap,
                      freeHeap * 100.0 / totalHeap);

        Serial.printf("PSRAM : %u / %u (%.1f%% free)\n",
                      freePsram,
                      totalPsram,
                      freePsram * 100.0 / totalPsram);

        Serial.printf("Largest Heap Block : %u\n",
                      heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));

        Serial.printf("Minimum Free Heap : %u\n",
                      ESP.getMinFreeHeap());

        Serial.printf("Internal Heap : %u\n",
                      heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    }
}