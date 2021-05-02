#pragma once

#include "memory.hpp"

class Memory;

class Timer {
   public:
    void init(Memory* memory);
    void tick(u8 cycles);

    void reset_div() {
        *div = 0;
        clocks = 0;
    }
    void set_enable(bool enabled) { this->enabled = enabled; }

    void set_frequency(u8 mode) {
        static constexpr u8 freqList[4] = {9, 3, 5, 7};
        bitFreq = freqList[mode];
    }

   private:
    Memory* memory;

    u8* div;
    u16 clocks;
    bool oldBitSet;
    bool timaScheduled;

    bool enabled;
    u8 bitFreq;
};