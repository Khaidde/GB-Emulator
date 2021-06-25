#pragma once

#include "cpu.hpp"
#include "general.hpp"
#include "memory.hpp"
#include "ppu.hpp"

class CPU;
class Memory;

#define PAUSE_EXEC(cpu, debugger, address) \
    if (cpu->PC == address) {              \
        debugger->pause_exec();            \
    }

#define PAUSE_EXEC_OCC(cpu, debugger, address, occurences) \
    static int occ = 0;                                    \
    if (cpu->PC == address) {                              \
        occ++;                                             \
        if (occ == occurences) {                           \
            debugger->pause_exec();                        \
            occ = 0;                                       \
        }                                                  \
    }

class Debugger {
public:
    void init(CPU* cpu, PPU* ppu, Memory* memory);

private:
    void print_reg(u16 address, const char* name);

public:
    void print_info();

    bool can_step();
    void step() { stepCnt++; }

    void pause_exec() { pause = true; }
    void continue_exec() { pause = false; }
    bool is_paused() { return pause; }

    bool check_fib_in_registers();

private:
    CPU* cpu;
    PPU* ppu;
    Memory* memory;

    u8 stepCnt;
    bool pause;
};
