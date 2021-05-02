#pragma once

#include "cartridge.hpp"
#include "debugger.hpp"
#include "input.hpp"
#include "timer.hpp"
#include "utils.hpp"

class Debugger;
class Input;
class Timer;

class Memory {
   public:
    enum RamRegisters : u16 {
        // I/O registers
        JOYP_REG = 0xFF00,

        // Timer
        DIV_REG = 0xFF04,
        TIMA_REG = 0xFF05,
        TMA_REG = 0xFF06,
        TAC_REG = 0xFF07,

        // Serial Data Transfer
        SB_REG = 0xFF01,
        SC_REG = 0xFF02,

        IF_REG = 0xFF0F,

        // LCD Control
        LCDC_REG = 0xFF40,

        // STAT
        STAT_REG = 0xFF41,

        // Graphics registers
        SCY_REG = 0xFF42,   // scroll y
        SCX_REG = 0xFF43,   // scroll x
        LY_REG = 0xFF44,    // current line from 0 -> 153
        LYC_REG = 0xFF45,   // line to compare against ly and trigger flag in stat
        DMA_REG = 0xFF46,   // DMA transer to OAM (160 machine cycles)
        BGP_REG = 0xFF47,   // background palette
        OBP0_REG = 0xFF48,  // object palette 0
        OBP1_REG = 0xFF49,  // object palette 1
        WY_REG = 0xFF4A,    // window y pos
        WX_REG = 0xFF4B,    // window x pos + 7

        IE_REG = 0xFFFF,
    };

    // Interrupts
    enum Interrupt : u8 {
        VBLANK_INT = 1 << 0,
        STAT_INT = 1 << 1,
        TIMER_INT = 1 << 2,
        SERIAL_INT = 1 << 3,
        JOYPAD_INT = 1 << 4,
    };

    void init(Debugger* debugger);
    void restart();
    void load_peripherals(Cartridge* cartridge, Input* input, Timer* timer);

    void request_interrupt(Interrupt interrupt);

    u8& ref(u16 address) { return mem[address]; }
    u8 read(u16 address);
    void write(u16 address, u8 val);
    void inc(u16 address);

   private:
    Debugger* debugger;

    Cartridge* cartridge;
    Input* input;
    Timer* timer;

    static constexpr int MEMORY_SIZE = 0x10000;
    u8 mem[MEMORY_SIZE];
};