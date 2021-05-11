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
    void push(T&& val) { data[(offset + size++) % capacity] = val; }
    T pop() {
        T val = data[offset];
        offset = (offset + 1) % capacity;
        size--;
        return val;
    }
};
