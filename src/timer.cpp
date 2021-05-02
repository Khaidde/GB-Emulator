#include "timer.hpp"

void Timer::init(Memory* memory) {
    this->memory = memory;
    div = &memory->ref(Memory::DIV_REG);
    clocks = 0xABCC;
    timaScheduled = false;

    oldBitSet = false;  // TODO verify this

    enabled = false;
    set_frequency(0);
}

void Timer::tick(u8 cycles) {
    // printf("\tinternal:%02x\n", clocks);
    // printf("\t%02x\n", bitFreq);
    for (int i = 0; i < cycles; i++) {
        clocks += 4;

        if (timaScheduled) {
            memory->write(Memory::TIMA_REG, memory->read(Memory::TMA_REG));
            memory->request_interrupt(Memory::TIMER_INT);
            timaScheduled = false;
        }

        bool newBitSet = (clocks >> bitFreq) & enabled;
        if (!newBitSet && oldBitSet) {
            memory->inc(Memory::TIMA_REG);
            if (memory->read(Memory::TIMA_REG) == 0) timaScheduled = true;
        }
        oldBitSet = newBitSet;

        *div = clocks >> 8;
    }
}
