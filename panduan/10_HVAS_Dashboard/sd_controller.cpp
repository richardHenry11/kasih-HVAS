#include "sd_controller.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <esp_display_panel.hpp>

static bool sdReady = false;

namespace {
constexpr int SD_MOSI = 11;
constexpr int SD_CLK = 12;
constexpr int SD_MISO = 13;
constexpr int SD_CS_EXIO = 4;
constexpr int SD_SS = -1;
}

bool sd_controller_init(void *board_ptr)
{
    auto *board = static_cast<esp_panel::board::Board *>(board_ptr);
    if (!board)
    {
        Serial.println("[SD] ERROR: board is null");
        sdReady = false;
        return false;
    }

    auto expander = board->getIO_Expander()->getBase();
    expander->digitalWrite(SD_CS_EXIO, LOW);

    SPI.setHwCs(false);
    SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_SS);

    sdReady = SD.begin(SD_SS);

    if (!sdReady)
    {
        Serial.println("[SD] CARD MOUNT FAILED");
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE)
    {
        Serial.println("[SD] NO CARD");
        sdReady = false;
        return false;
    }

    Serial.println("[SD] READY");
    return true;
}

bool sd_controller_is_ready()
{
    return sdReady;
}
