#pragma once

#include "general.hpp"

class Debugger;
class Memory;

class Timer {
public:
    Timer(Memory& memory);
    void restart();
    void set_debugger(Debugger& debugger) { this->debugger = &debugger; }

    void emulate_cycle();
    void try_trigger_tima();

    void reset_div();
    void write_tima(u8 newTima);
    void write_tma(u8 newTma);
    void write_tac(u8 newTac);

private:
    friend class Debugger;
    Debugger* debugger;

    Memory* memory;

    u8* div;
    u8* tima;
    u8* tma;
    u16 cycles;
    bool oldBitSet;
    bool timaIntrSchedule;

    bool timaWrote;     // Whether or not tima was written to by cpu this cycle
    bool timaReloaded;  // Whether or not tima was reloaded this cycle

    bool enabled;
    u8 bitFreq;
};
