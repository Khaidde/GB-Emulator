#pragma once

#include "cartridge.hpp"
#include "debugger.hpp"
#include "input.hpp"
#include "ppu.hpp"
#include "timer.hpp"
#include "utils.hpp"

#include <memory>

class Debugger;
class Timer;
class PPU;

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
    Memory();
    void restart();
    void set_debugger(Debugger* debugger);
    void set_peripherals(Input* input, Timer* timer, PPU* ppu);
    void load_cartridge(const char* romPath);

    void request_interrupt(Interrupt interrupt);

    u8& ref(u16 addr);
    u8 read(u16 addr);
    void write(u16 addr, u8 val);

    void schedule_read(u16 addr, u8* dest, u8 cycle);
    void schedule_write(u16 addr, u8* val, u8 cycle);
    void set_bus_addr(u16 addr);

    void emulate_dma_cycle();

    void emulate_cycle();
    void reset_cycles();

   private:
    Debugger* debug;

    std::unique_ptr<Cartridge> cartridge;

    Input* input;
    Timer* timer;
    PPU* ppu;

    static constexpr int MEMORY_SIZE = 0x10000;
    u8 mem[MEMORY_SIZE];

    Queue<MemoryOp, 16> scheduledMemoryOps;
    u8 cycleCnt;

    bool oamInaccessible;
    bool scheduleDma;
    u16 dmaStartAddr;
    u8 dmaCycleCnt;
};