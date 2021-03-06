#pragma once

#include "general.hpp"

class Memory;

class Input {
public:
    Input(Memory& memory);
    void restart();
    void handle_input(bool pressed, u8 key);
    u8 get_key_state(u8 keyReg);

private:
    Memory* memory;
    u8 keyState;
};
