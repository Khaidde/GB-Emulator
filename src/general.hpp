#pragma once

#include <cstdint>
#include <stdexcept>
#include <cstring>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

using s8 = int8_t;
using s16 = int16_t;

#define fatal(...)                                                                     \
    char buff[0xFF] = "FATAL ERROR: ";                                                 \
    snprintf(buff + 13 * sizeof(char), sizeof(buff) - 13 * sizeof(char), __VA_ARGS__); \
    throw std::runtime_error(buff)

namespace FileManagement {

bool is_path_extension(const char* romPath, const char* extension);

}  // namespace FileManagement

template <typename T, int capacity>
struct Queue {
    void clear() {
        offset = 0;
        size = 0;
    }
    T* get(u8 index) { return &data[(index + offset) % capacity]; }
    void set(u8 index, T&& val) { data[(index + offset) % capacity] = val; }
    void push_tail(T&& val) { data[(offset + size++) % capacity] = val; }
    T pop_head() {
        T val = data[offset];
        offset = (offset + 1) % capacity;
        size--;
        return val;
    }
    T head() { return data[offset]; }

    u8 offset = 0;
    u8 size = 0;
    T data[capacity];
};

namespace Constants {

constexpr const char* TITLE = "GameBoy Emulator";
constexpr int WIDTH = 160;
constexpr int HEIGHT = 144;

constexpr double MS_PER_FRAME = 16.7427;

constexpr double MASTER_VOLUME = 3;
constexpr int SAMPLE_RATE = 48000;

}  // namespace Constants

enum class JoypadButton : char {
    RIGHT_BTN = 0,
    LEFT_BTN = 1,
    UP_BTN = 2,
    DOWN_BTN = 3,
    A_BTN = 4,
    B_BTN = 5,
    SELECT_BTN = 6,
    START_BTN = 7,

    NONE = 8,
};

namespace IOReg {
enum : u16 {
    // I/O registers
    JOYP_REG = 0xFF00,

    // Serial Data Transfer
    SB_REG = 0xFF01,
    SC_REG = 0xFF02,

    // Timer
    DIV_REG = 0xFF04,
    TIMA_REG = 0xFF05,
    TMA_REG = 0xFF06,
    TAC_REG = 0xFF07,

    // Interrupt flags
    IF_REG = 0xFF0F,

    // Sound registers
    NR10_REG = 0xFF10,
    NR11_REG = 0xFF11,
    NR12_REG = 0xFF12,
    NR13_REG = 0xFF13,
    NR14_REG = 0xFF14,

    NR21_REG = 0xFF16,
    NR22_REG = 0xFF17,
    NR23_REG = 0xFF18,
    NR24_REG = 0xFF19,

    NR30_REG = 0xFF1A,
    NR31_REG = 0xFF1B,
    NR32_REG = 0xFF1C,
    NR33_REG = 0xFF1D,
    NR34_REG = 0xFF1E,

    NR41_REG = 0xFF20,
    NR42_REG = 0xFF21,
    NR43_REG = 0xFF22,
    NR44_REG = 0xFF23,

    NR50_REG = 0xFF24,
    NR51_REG = 0xFF25,
    NR52_REG = 0xFF26,

    WAVE_TABLE_START_REG = 0xFF30,

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
}  // namespace IOReg

enum class Interrupt : u8 {
    VBLANK_INT = 1 << 0,
    STAT_INT = 1 << 1,
    TIMER_INT = 1 << 2,
    SERIAL_INT = 1 << 3,
    JOYPAD_INT = 1 << 4,
};