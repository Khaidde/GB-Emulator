#pragma once

#include "general.hpp"

class Memory;
class Debugger;

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
    enum FetchState {
        READ_TILE_ID,
        READ_TILE_0,
        READ_TILE_1,
        PUSH,
        SLEEP,
    };
    u8 curFetchState;
    static constexpr FetchState FETCH_STATES[] = {
        SLEEP, READ_TILE_ID, SLEEP, READ_TILE_0, SLEEP, READ_TILE_1, PUSH, PUSH,
    };
    Sprite* curSprite;
    u8 tileX;

    bool windowMode;
    bool windowXEnable;
    u8 windowY;

    u16 tileByteAddr;
    u8 tileAttribs;
    u8 data0;
    u8 data1;
};

enum DMGPalette : u8 {
    BGP = 0,
    OBP0 = 1,
    OBP1 = 2,
};
struct FIFOData {
    u8 colIndex;
    u8 paletteNum;
    bool bgPriority;
};

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

enum PPUMode : u8 {
    H_BLANK_MODE = 0,
    V_BLANK_MODE = 1,
    OAM_MODE = 2,
    LCD_MODE = 3,
};

enum class PPUState {
    OAM_3,
    OAM_4,
    OAM_PRE_1,
    OAM,
    LCD_4,
    LCD_5,
    LCD,
    H_BLANK_4,
    H_BLANK,

    V_BLANK_144_4,
    V_BLANK_145_152_4,
    V_BLANK_153_4,
    V_BLANK_153_8,
    V_BLANK_153_12,
    V_BLANK,
};

class PPU {
public:
    static constexpr int OAM_SEARCH_CLOCKS = 80;

    static constexpr int SCAN_LINE_CLOCKS = 456;
    static constexpr int V_BLANK_END_LINE = 154;

    static constexpr int TOTAL_CLOCKS = SCAN_LINE_CLOCKS * V_BLANK_END_LINE;

    PPU(Memory& memory);
    void restart();
    void set_debugger(Debugger& debug) { this->debugger = &debug; }

    void write_register(u16 addr, u8 val);
    void write_vbk(u8 val);

    void write_vram(u16 addr, u8 val);
    u8 read_vram(u16 addr);

    bool is_oam_read_blocked();
    bool is_oam_write_blocked();

    void render(u32* pixelBuffer);
    void emulate_clock();

private:
    void set_stat_mode(PPUMode statMode);
    void update_coincidence();
    void clear_coincidence();
    void try_lyc_intr();
    void try_mode_intr(u8 statMode);
    void clear_stat_mode_intr(PPUMode statMode);

    void try_trigger_stat();

    void handle_pixel_render();

    void background_fetch();
    void sprite_fetch();

    bool get_lcdc_flag(LCDCFlag flag);
    bool is_window_enabled();

    friend class Debugger;
    Debugger* debugger;
    Memory* memory;

    u8* lcdc;
    u8* stat;
    u8* scy;
    u8* scx;
    u8* ly;
    u8* lyc;
    u8* wy;
    u8* wx;

    bool bgAutoIncrement;
    u8 bgPaletteIndex;
    u8 bgPaletteData[64];  // 8 bytes per palette, 8 palettes

    bool obAutoIncrement;
    u8 obPaletteIndex;
    u8 obPaletteData[64];

    static constexpr u16 OAM_START_ADDR = 0xFE00;
    u8* oamAddrBlock;

    static constexpr u16 VRAM_START_ADDR = 0x8000;
    using VRAMBank = u8[0x2000];
    VRAMBank* curVramBank;
    VRAMBank tileMapVram;     // VRAM BANK 0
    VRAMBank tileAttribVram;  // VRAM BANK 1

    using FrameBuffer = u32[160 * 144];
    bool bufferSel;
    FrameBuffer frameBuffers[2];

    short lineClocks;
    short clockCnt;
    PPUState curPPUState;

    SpriteList spriteList;

    static constexpr u8 TILESET_SIZE = 32;  // width and height of vram tileset
    static constexpr u8 TILE_MEM_LEN = 16;
    static constexpr u8 TILE_PX_SIZE = 8;  // width and height of a tile in pixels
    Fetcher fetcher;

    static constexpr u8 FIFO_SIZE = 16;
    Queue<FIFOData, FIFO_SIZE> bgFifo;
    Queue<FIFOData, FIFO_SIZE> spriteFifo;

    u8 lockedSCX;

    short curPixelX;
    u8 line;

    u8 statIntrFlags;  // 3: ly=lyc, 2: oam, 1: v-blank, 0: h-blank
    u8 prevIntrFlags;
    u8 internalStatEnable;

    bool oamBlockRead;
    bool oamBlockWrite;
    bool vramBlockRead;
    bool vramBlockWrite;

    // ICE_CREAM_GB const u32 baseColors[4] = {0xFFFFF6D3, 0xFFF9A875, 0xFFEB6B6F, 0xFF7C3F58};
    // COLD_FIRE_GB const u32 baseColors[4] = {0xFFF6C6A8, 0xFFD17C7C, 0xFF5B768D, 0xFF46425E};
    // SEA_SALT_ICE_CREAM const u32 baseColors[4] = {0xFFFFF6D3, 0xFF8BE6FF, 0xFF608ECF,
    // 0xFF3336E8}; MIST_GB
    const u32 baseColors[4] = {0xFFC4F0C2, 0xFF5AB9A8, 0xFF1E606E, 0xFF2D1B00};
    // CANDYPOP const u32 baseColors[4] = {0xFFEEBFF5, 0xFF9E81D0, 0xFF854576, 0xFF301221};
    // CAVE4 const u32 baseColors[4] = {0xFFE4CBBF, 0xFF938282, 0xFF4F4E80, 0xFF2C0016};
    const u32 BLANK_COLOR = 0xFF101010;
};
