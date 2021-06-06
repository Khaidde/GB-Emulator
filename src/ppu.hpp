#pragma once

#include "general.hpp"

class Memory;

struct Sprite {
    u8 y;
    u8 x;
    u8 tileID;
    u8 flags;
};

struct SpriteList {
    void clear();
    void add(Sprite&& sprite);
    void remove(u8 index);

    static constexpr u8 MAX_SPRITES = 10;
    Sprite data[MAX_SPRITES];
    u16 removedBitField = 0;
    u8 size = 0;
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

    u8 tileX;

    u16 tileRowAddrOff;
    u8 data0;
    u8 data1;
};

class PPU {
   public:
    static constexpr int OAM_SEARCH_CLOCKS = 80;

    static constexpr int LCD_AND_H_BLANK_CLOCKS = 376;

    static constexpr int SCAN_LINE_CLOCKS = OAM_SEARCH_CLOCKS + LCD_AND_H_BLANK_CLOCKS;
    static constexpr int V_BLANK_END_LINE = 154;

    static constexpr int TOTAL_CLOCKS = SCAN_LINE_CLOCKS * V_BLANK_END_LINE;

    PPU(Memory* memory);
    void restart();
    void render(u32* pixelBuffer);
    void emulate_clock();

    void update_stat();
    void update_coincidence();
    void trigger_stat_intr();

    bool is_vram_blocked();
    bool is_oam_blocked();

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

    static constexpr u16 OAM_START_ADDR = 0xFE00;
    u8* oamAddrBlock;

    static constexpr u16 VRAM_START_ADDR = 0x8000;
    u8* vramAddrBlock;

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
    u8 line;

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

    enum Mode : u8 {
        H_BLANK = 0,
        V_BLANK = 1,
        OAM_SEARCH = 2,
        LCD_TRANSFER = 3,
    } mode;
    u8 statIntrFlags;  // 3: ly=lyc, 2: oam, 1: v-blank, 0: h-blank
    bool statTrigger;
    int modeSwitchClocks;
    int lylycTriggerClock;

    // ICE_CREAM_GB const u32 baseColors[4] = {0xFFFFF6D3, 0xFFF9A875, 0xFFEB6B6F, 0xFF7C3F58};
    // COLD_FIRE_GB const u32 baseColors[4] = {0xFFF6C6A8, 0xFFD17C7C, 0xFF5B768D, 0xFF46425E};
    // SEA_SALT_ICE_CREAM const u32 baseColors[4] = {0xFFFFF6D3, 0xFF8BE6FF, 0xFF608ECF, 0xFF3336E8};
    // MIST_GB
    const u32 baseColors[4] = {0xFFC4F0C2, 0xFF5AB9A8, 0xFF1E606E, 0xFF2D1B00};
    // CANDYPOP const u32 baseColors[4] = {0xFFEEBFF5, 0xFF9E81D0, 0xFF854576, 0xFF301221};
    // CAVE4 const u32 baseColors[4] = {0xFFE4CBBF, 0xFF938282, 0xFF4F4E80, 0xFF2C0016};
    const u32 BLANK_COLOR = 0xFF101010;

    void handle_pixel_push();

    void background_fetch();
    void sprite_fetch();

    bool get_lcdc_flag(LCDCFlag flag);

    void set_mode(Mode changeToMode);

    u32 get_color(FIFOData&& data);
};