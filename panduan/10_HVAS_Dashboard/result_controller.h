#pragma once

#include <Arduino.h>

enum ResultAction
{
    RESULT_ACTION_NONE = 0,
    RESULT_ACTION_PRINT_ONLY,
    RESULT_ACTION_SAVE_ONLY,
    RESULT_ACTION_PRINT_AND_SAVE
};

void result_controller_init();

bool result_print();
bool result_save();

bool result_print_and_save();

bool result_execute(ResultAction action);
