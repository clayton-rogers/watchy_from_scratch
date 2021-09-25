#pragma once

enum class Button {
    NONE,
    MENU,
    BACK,
    UP,
    DOWN,
};

Button get_next_button();
