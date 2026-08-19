#pragma once

#include <Arduino.h>

/*
 * HVAS BLE interface
 *
 * ESP32-S3 = BLE Peripheral / GATT Server
 * Mobile    = BLE Central / GATT Client
 *
 * Command characteristic: WRITE
 * Response characteristic: NOTIFY
 */
void ble_init();
void ble_task();
bool ble_is_connected();
