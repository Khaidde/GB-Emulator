#pragma once

#include "utils.hpp"

class Memory;

class Timer {
   public:
    void init(Memory* memory);
    void emulate_clock();

    void reset_div();
    void set_enable(bool isEnabled);
    void set_frequency(u8 mode);

   private:
    Memory* mem;

    u8* div;
    u8* tima;
    u16 clocks;
    bool oldBitSet;
    bool timaScheduled;

    bool enabled;
    u8 bitFreq;
};