#pragma once

#include "general.hpp"

class Debugger;
class Memory;

class Serial {
public:
    Serial(Memory& memory);
    void restart();
    void set_debugger(Debugger& debugger) { this->debugger = &debugger; }

    void emulate_cycle();
    void trigger_transfer();

private:
    friend class Debugger;
    Debugger* debugger;

    Memory* memory;

    u8* sb;
    u8* sc;

    u16 cycles;
    u16 bitsToTransfer;
};
