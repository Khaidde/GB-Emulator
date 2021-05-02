#pragma once

#include <SDL.h>
#include <stdio.h>

#include "cpu.hpp"
#include "memory.hpp"
#include "utils.hpp"

class CPU;
class Memory;

class Debugger {
   public:
    void init(CPU* cpu, Memory* memory);

    void update_instr(u16 opPC);
    void inc_instr_bytes() { instrByteLen++; }

    void print_reg(u16 address, const char* name);
    void print_instr();
    void handle_function_key(SDL_Event e);

    // Returns whether or not a step was successful
    bool step();

    void pause_exec() { pause = true; }
    void continue_exec() { pause = false; }
    bool is_paused() { return pause; }

   private:
    CPU* cpu;
    Memory* memory;

    u8 stepCnt;
    bool pause;

    u16 opPC;
    u8 instrByteLen;
};