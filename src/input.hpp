#pragma once

#include "utils.hpp"

class Memory;

class Input {
   public:
    void init(Memory* memory) {
        this->memory = memory;
        keystate = 0xFF;
    }
    void handle_input(bool pressed, u8 key);
    u8 get_key_state(bool btnSelect, bool dirSelect);

   private:
    Memory* memory;
    u8 keystate;
};