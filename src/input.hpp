#pragma once

#include <SDL.h>

#include "memory.hpp"
#include "utils.hpp"

class Memory;

class Input {
   public:
    void init(Memory* memory) {
        this->memory = memory;
        keystate = 0xFF;
    }
    void handle_input(SDL_Event keyEvent);
    u8 get_key_state(bool btnSelect, bool dirSelect);

   private:
    Memory* memory;
    u8 keystate;
};