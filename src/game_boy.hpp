#pragma once

#include "apu.hpp"
#include "cartridge.hpp"
#include "cpu.hpp"
#include "debugger.hpp"
#include "input.hpp"
#include "memory.hpp"
#include "ppu.hpp"
#include "timer.hpp"

class GameBoy {
   public:
    GameBoy();
    void load(const char* romPath);
    void set_debugger(Debugger& debugger) {
        this->debugger = &debugger;
        debugger.init(&cpu, &memory);

        cpu.set_debugger(debugger);
        memory.set_debugger(debugger);
    }

    void handle_key_code(bool pressed, JoypadButton button) { input.handle_input(pressed, (char)button); }
    void emulate_frame(u32* screenBuffer, s16* sampleBuffer, u16 sampleLen);

   private:
    Debugger* debugger;

    int totalTCycles;

    Input input;
    Timer timer;
    PPU ppu;
    APU apu;
    Memory memory;
    CPU cpu;
};