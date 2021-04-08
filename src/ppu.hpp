#pragma once

#include "memory.hpp"
#include "utils.hpp"

class PPU {
   public:
    PPU(Memory* memory);
    ~PPU();

    void render(u32* pixelBuffer);

    static constexpr u16 TOTAL_CYCLES = 17556;
    static constexpr u8 OAM_SEARCH_CYCLES = 20;
    static constexpr u8 LCD_TRANSFER_CYCLES = 43;
    static constexpr u8 H_BLANK_CYCLES = 51;
    static constexpr u8 SCAN_LINE_CYCLES = OAM_SEARCH_CYCLES + LCD_TRANSFER_CYCLES + H_BLANK_CYCLES;
    static constexpr u8 V_BLANK_END_LINE = 154;
    void proccess(u8 cycles);

   private:
    Memory* memory;

    u32* frameBuffer;

    const u32 baseColors[4] = {0xFFFF6D3, 0xFFEB6B6F, 0xFF7C3F58, 0xFFF9A875};
    const u32 BLANK_COLOR = 0xFF010101;
    u32 get_color(u8 col);

    enum Mode : u8 {
        H_BLANK = 0,
        V_BLANK = 1,
        OAM_SEARCH = 2,
        LCD_TRANSFER = 3,
    };
    Mode mode = OAM_SEARCH;
    u8 drawCycles = 0;

    void draw_bg(u8 line);
};