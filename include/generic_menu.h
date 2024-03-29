#pragma once

#include <Arduino.h>

typedef void (*menu_handler_ptr)();

// For menu items that you don't want to populate
void null_menu();

// Both labels and callbacks should be size 6
void handle_generic_menu(const char** menu_labels,
                         menu_handler_ptr* menu_callbacks);
