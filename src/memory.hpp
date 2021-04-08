#pragma once

#include "cartridge.hpp"
#include "utils.hpp"

class Memory {
   public:
    static constexpr u16 VRAM_TILE_DATA = 0x8000;

    // I/O registers
    static constexpr u16 JOYP_REG = 0xFF00;

    // LCD Control
    static constexpr u16 LCDC_REG = 0xFF40;

    // LCD Position and Scrolling
    static constexpr u16 SCY_REG = 0xFF42;  // scroll y
    static constexpr u16 SCX_REG = 0xFF43;  // scroll x
    static constexpr u16 LY_REG = 0xFF44;   // current line from 0 -> 153
    static constexpr u16 LYC_REG = 0xFF45;  // line to compare against ly and trigger flag in stat
    static constexpr u16 DMA_REG = 0xFF46;  // DMA transer to OAM (160 machine cycles)
    static constexpr u16 BGP_REG = 0xFF47;  // background palette
    static constexpr u16 WY_REG = 0xFF4A;   // window y pos
    static constexpr u16 WX_REG = 0xFF4B;   // window x pos + 7

    // Interrupts
    enum Interrupt : u8 {
        VBLANK_INT = 1 << 0,
        STAT_INT = 1 << 1,
        TIMER_INT = 1 << 2,
        SERIAL_INT = 1 << 3,
        JOYPAD_INT = 1 << 4,
    };
    static constexpr u16 IF_REG = 0xFF0F;
    static constexpr u16 IE_REG = 0xFFFF;

    void startup();
    void load_cartridge(Cartridge* cartridge);
    void request_interrupt(Interrupt interrupt);
    u8 read(u16 address);
    void write(u16 address, u8 val);
    void inc(u16 address);

   private:
    Cartridge* cartridge;

    static constexpr int MEMORY_SIZE = 0x10000;
    u8 mem[MEMORY_SIZE];
};