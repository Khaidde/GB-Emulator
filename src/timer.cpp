#include "timer.hpp"

#include "memory.hpp"

void Timer::init(Memory* memory) {
    this->memory = memory;
    div = &memory->ref(IOReg::DIV_REG);
    tima = &memory->ref(IOReg::TIMA_REG);
    clocks = 0xABCC;
    timaScheduled = false;

    oldBitSet = false;

    enabled = false;
    set_frequency(0);
}

void Timer::emulate_clock() {
    clocks++;

    if (timaScheduled) {
        memory->write(IOReg::TIMA_REG, memory->read(IOReg::TMA_REG));
        memory->request_interrupt(Interrupt::TIMER_INT);
        timaScheduled = false;
    }

    bool newBitSet = (clocks >> bitFreq) & enabled;
    if (!newBitSet && oldBitSet) {
        (*tima)++;
        if (memory->read(IOReg::TIMA_REG) == 0) timaScheduled = true;
    }
    oldBitSet = newBitSet;

    *div = clocks >> 8;
}
