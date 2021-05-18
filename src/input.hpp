#pragma once

#include "utils.hpp"

class Memory;

class Input {
   public:
    void init(Memory* memory) {
        this->mem = memory;
        keyState = 0xFF;
    }
    void handle_input(bool pressed, u8 key);
    u8 get_key_state(u8 keyReg);

   private:
    Memory* mem;
    u8 keyState;
};