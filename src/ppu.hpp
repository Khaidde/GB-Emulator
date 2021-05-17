#pragma once

#include "utils.hpp"

class Memory;

struct Sprite {
    u8 y;
    u8 x;
    u8 tileID;
    u8 flags;
};

struct SpriteList {
    static constexpr u8 MAX_SPRITES = 10;
    Sprite data[MAX_SPRITES];
    u16 removedBitField;
    u8 size = 0;

    void clear() {
        size = 0;
        removedBitField = 0;
    }
    void add(Sprite&& sprite) { data[size++] = sprite; }
    void remove(u8 index) { removedBitField |= 1 << index; }
};

struct Fetcher {
    bool doFetch;
    enum {
        READ_TILE_ID,
        READ_TILE_0,
        READ_TILE_1,
        PUSH,
    } fetchState;
    Sprite* curSprite;

    bool windowMode;

    u16 tileRowAddr;
    u8 data0;
    u8 data1;

    u8 tileX;
};

class PPU {
   public:
    static constexpr int OAM_SEARCH_CLOCKS = 80;

    static constexpr int LCD_AND_H_BLANK_CLOCKS = 376;

    static constexpr int SCAN_LINE_CLOCKS = OAM_SEARCH_CLOCKS + LCD_AND_H_BLANK_CLOCKS;
    static constexpr int V_BLANK_END_LINE = 154;

    static constexpr int TOTAL_CLOCKS = SCAN_LINE_CLOCKS * V_BLANK_END_LINE;

    void init(Memory* memory);
    void render(u32* pixelBuffer);
    void emulate_clock();

    void update_stat();
    void trigger_stat_intr();

    u8 read_ly();

   private:
    Memory* memory;

    u8* lcdc;
    u8* stat;
    u8* scy;
    u8* scx;
    u8* ly;
    u8* lyc;
    u8* wy;
    u8* wx;

    using FrameBuffer = u32[160 * 144];
    bool bufferSel;
    FrameBuffer frameBuffers[2];

    u16 drawClocks;

    SpriteList spriteList;

    static constexpr u8 TILESET_SIZE = 32;  // width and height of vram tileset
    static constexpr u8 TILE_MEM_LEN = 16;
    static constexpr u8 TILE_PX_SIZE = 8;  // width and height of a tile in pixels
    static constexpr u16 VRAM_TILE_DATA_0 = 0x8000;
    static constexpr u16 VRAM_TILE_DATA_1 = 0x9000;
    Fetcher fetcher;

    struct FIFOData {
        u8 colIndex;
        bool isSprite;
        u16 palleteAddr;
    };
    static constexpr u8 FIFO_SIZE = 16;
    Queue<FIFOData, FIFO_SIZE> pxlFifo;

    u8 numDiscardedPixels;
    u8 curPixelX;

    enum class LCDCFlag : u8 {
        BG_WINDOW_ENABLE = 0,
        SPRITE_ENABLE = 1,
        SPRITE_SIZE = 2,
        BG_TILE_SELECT = 3,
        TILE_DATA_SELECT = 4,
        WINDOW_ENABLE = 5,
        WINDOW_TILE_SELECT = 6,
        LCD_ENABLE = 7,
    };

    enum class STATIntrFlag : char {
        LYC_LY_INTR = 6,
        MODE_2_INTR = 5,
        MODE_1_INTR = 4,
        MODE_0_INTR = 3,
    };

    enum Mode : u8 {
        H_BLANK = 0,
        V_BLANK = 1,
        OAM_SEARCH = 2,
        LCD_TRANSFER = 3,
    } mode;
    bool statTrigger;
    bool curCycleStatTrigger;
    int modeSwitchClocks;

    const u32 baseColors[4] = {0xFFFFF6D3, 0xFFF9A875, 0xFFEB6B6F, 0xFF7C3F58};
    const u32 BLANK_COLOR = 0xFF101010;

    void background_fetch();
    void sprite_fetch();

    bool get_lcdc_flag(LCDCFlag flag);

    void set_mode(Mode mode);

    u32 get_color(FIFOData&& data);

    u16 get_sprite_addr(u8 index);
};