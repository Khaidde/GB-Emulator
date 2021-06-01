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
    void print_cartridge_info();

    void set_debugger(Debugger& debugger);
    void handle_key_code(bool pressed, JoypadButton button) { input.handle_input(pressed, (char)button); }

    void emulate_frame();
    void emulate_frame(u32* screenBuffer, s16* sampleBuffer, u16 sampleLen);

    // TODO there might be several serial out values per frame
    char get_serial_out();

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