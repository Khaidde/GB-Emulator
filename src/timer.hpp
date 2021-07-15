#pragma once

#include "general.hpp"

class Memory;

class Timer {
public:
    Timer(Memory& memory);
    void restart();
    void emulate_clock();

    void reset_div();
    void write_tima(u8 newTima);
    void write_tma(u8 newTma);
    void write_tac(u8 newTac);

private:
    Memory* memory;

    u8* div;
    u8* tima;
    u8* tma;
    u16 clocks;
    bool oldBitSet;
    u8 timaScheduleCnt;

    bool timaWrote;     // Whether or not tima was written to by cpu this cycle
    bool timaReloaded;  // Whether or not tima was reloaded this cycle

    bool enabled;
    u8 bitFreq;
};
