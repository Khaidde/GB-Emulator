#pragma once

#include <stdint.h>
#include <stdexcept>

#define fatal(...)                                                                     \
    char buff[0xFF] = "FATAL ERROR: ";                                                 \
    snprintf(buff + 13 * sizeof(char), sizeof(buff) - 13 * sizeof(char), __VA_ARGS__); \
    throw std::runtime_error(buff);

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

using s8 = int8_t;

template <class T, int capacity>
struct Queue {
    u8 offset = 0;
    u8 size = 0;
    T data[capacity];

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
    T tail() { return data[(offset + size) % capacity]; }
};

namespace IOReg {
enum : u16 {
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
}  // namespace IOReg

enum class Interrupt : u8 {
    VBLANK_INT = 1 << 0,
    STAT_INT = 1 << 1,
    TIMER_INT = 1 << 2,
    SERIAL_INT = 1 << 3,
    JOYPAD_INT = 1 << 4,
};