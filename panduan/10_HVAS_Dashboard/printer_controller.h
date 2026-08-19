#pragma once

#include <Arduino.h>

bool printer_init();

bool printer_is_ready();
bool printer_check();

bool printer_print_line(const char *text);
bool printer_print_result();

void printer_feed(uint8_t lines);