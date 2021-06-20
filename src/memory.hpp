#pragma once

#include <memory>

#include "apu.hpp"
#include "cartridge.hpp"
#include "debugger.hpp"
#include "general.hpp"
#include "input.hpp"
#include "ppu.hpp"
#include "timer.hpp"

class Debugger;
class Timer;
class PPU;
class APU;

struct MemoryOp {
    u8 cycle;

    u16 addr;
    bool isWrite;
    union {
        u8* readDest;
        u8* writeVal;
    };
};

class Memory {
public:
    void restart();
    void set_cartridge(Cartridge* cartridge) { this->cartridge = cartridge; }
    void set_debugger(Debugger& debugger) { this->debugger = &debugger; }
    void set_input(Input& in) { this->input = &in; }
    void set_timer(Timer& time) { this->timer = &time; }
    void set_ppu(PPU& p) { this->ppu = &p; }
    void set_apu(APU& a) { this->apu = &a; }

    void request_interrupt(Interrupt interrupt);

    u8& ref(u16 addr);
    u8 read(u16 addr);
    void write(u16 addr, u8 val);

    void schedule_read(u16 addr, u8* dest, u8 cycle);
    void schedule_write(u16 addr, u8* val, u8 cycle);

    void emulate_dma_cycle();

    void emulate_cycle();
    void reset_cycles();

private:
    Debugger* debugger;

    Cartridge* cartridge;

    Input* input;
    Timer* timer;
    PPU* ppu;
    APU* apu;

    static constexpr int MEMORY_SIZE = 0x10000;
    u8 mem[MEMORY_SIZE];

    Queue<MemoryOp, 16> scheduledMemoryOps;
    u8 cycleCnt;

    bool dmaInProgress;
    bool scheduleDma;
    u16 dmaStartAddr;
    u8 dmaCycleCnt;
};
