#pragma once

#include "cpu.hpp"
#include "memory.hpp"
#include "utils.hpp"

class CPU;
class Memory;

class Debugger {
   public:
    void init(CPU* c, Memory* memory);

    void print_reg(u16 address, const char* name);
    void print_info();

    bool can_step();
    void step() { stepCnt++; }

    void pause_exec() { pause = true; }
    void continue_exec() { pause = false; }
    bool is_paused() { return pause; }

   private:
    CPU* cpu;
    Memory* mem;

    u8 stepCnt;
    bool pause;
};