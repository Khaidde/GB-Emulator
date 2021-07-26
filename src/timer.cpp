#include "timer.hpp"

#include "memory.hpp"

Timer::Timer(Memory& memory) : memory(&memory) {
    div = &memory.ref(IOReg::DIV_REG);
    tima = &memory.ref(IOReg::TIMA_REG);
    tma = &memory.ref(IOReg::TMA_REG);
}

void Timer::restart() {
    // Since nops have a precision of 4 cycles, there's
    // no real way to know the exact value through test roms
    // Note that cpu is 4 cycles late due to initial fetch cycle
    if (memory->is_CGB()) {
        // 0x2674 <= clocks < 0x2678
        cycles = 0x2674 >> 2;
    } else {
        // 0xABC8 <= clocks <= 0xABCC
        cycles = 0xABC8 >> 2;
    }
    timaIntrSchedule = false;

    oldBitSet = false;

    timaWrote = false;
    timaReloaded = false;

    write_tac(0xF8);
}

void Timer::emulate_cycle() {
    cycles++;
    *div = cycles >> 6;

    if (timaReloaded) {
        timaReloaded = false;
    }
    if (timaIntrSchedule) {
        if (timaWrote) {
            timaWrote = false;
        } else {
            *tima = *tma;
            memory->request_interrupt(Interrupt::TIMER_INT);
            timaReloaded = true;
        }
        timaIntrSchedule = false;
    }
    try_trigger_tima();
}

void Timer::try_trigger_tima() {
    bool newBitSet = (cycles >> bitFreq) & enabled;
    if (!newBitSet && oldBitSet) {
        (*tima)++;
        if (*tima == 0) {
            timaIntrSchedule = true;
        }
    }
    oldBitSet = newBitSet;
}

void Timer::reset_div() {
    *div = 0;
    cycles = 0;
}

void Timer::write_tima(u8 newTima) {
    if (timaIntrSchedule) {
        timaWrote = true;
    }
    if (!timaReloaded) {
        *tima = newTima;
    }
}

void Timer::write_tma(u8 newTma) {
    *tma = newTma;
    if (timaReloaded) {
        *tima = *tma;
    }
}

static constexpr u8 freqList[4] = {7, 1, 3, 5};
void Timer::write_tac(u8 newTac) {
    bitFreq = freqList[newTac & 0x3];
    this->enabled = (newTac & 0x4) != 0;
    try_trigger_tima();
}
