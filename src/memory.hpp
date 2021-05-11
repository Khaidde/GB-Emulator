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

class Memory {
   public:
    Memory();
    void restart();
    void set_debugger(Debugger* debugger);
    void set_peripherals(Input* input, Timer* timer, PPU* ppu);
    void load_cartridge(const char* romPath);

    u8& ref(u16 addr);
    u8 read(u16 addr);
    void write(u16 addr, u8 val);

    void request_interrupt(Interrupt interrupt);

   private:
    Debugger* debugger;

    std::unique_ptr<Cartridge> cartridge;

    Input* input;
    Timer* timer;
    PPU* ppu;

    static constexpr int MEMORY_SIZE = 0x10000;
    u8 mem[MEMORY_SIZE];
};