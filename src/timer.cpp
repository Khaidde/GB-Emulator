#include "timer.hpp"

#include "memory.hpp"

Timer::Timer(Memory& memory) : memory(&memory) {
    div = &memory.ref(IOReg::DIV_REG);
    tima = &memory.ref(IOReg::TIMA_REG);
}

void Timer::restart() {
    // Since nops have a precision of 4 cycles, there's
    // no real way to know the exact value through test roms
    if (memory->is_CGB()) {
        clocks = 0x267B;
        // 0x2678 <= clocks < 0x267C
    } else {
        clocks = 0xABCC;
        // 0xABCC <= clocks <= 0xABD0
    }
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

void Timer::reset_div() {
    *div = 0;
    clocks = 0;
}
void Timer::set_enable(bool isEnabled) { this->enabled = isEnabled; }

void Timer::set_frequency(u8 mode) {
    static constexpr u8 freqList[4] = {9, 3, 5, 7};
    bitFreq = freqList[mode];
}
