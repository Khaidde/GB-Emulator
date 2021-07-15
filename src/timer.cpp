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
    if (memory->is_CGB()) {
        clocks = 0x267B;
        // 0x2678 <= clocks < 0x267C
    } else {
        clocks = 0xABCC;
        // 0xABCC <= clocks <= 0xABD0
    }
    timaScheduleCnt = 0;

    oldBitSet = false;

    timaWrote = false;
    timaReloaded = false;

    write_tac(0xF8);
}

void Timer::emulate_clock() {
    clocks++;

    if (timaReloaded) {
        timaReloaded = false;
    }
    if (timaScheduleCnt > 0) {
        timaScheduleCnt--;
        if (timaScheduleCnt == 0) {
            if (timaWrote) {
                timaWrote = false;
            } else {
                *tima = *tma;
                memory->request_interrupt(Interrupt::TIMER_INT);
                timaReloaded = true;
            }
        }
    }

    bool newBitSet = (clocks >> bitFreq) & enabled;
    if (!newBitSet && oldBitSet) {
        (*tima)++;
        if (*tima == 0) {
            timaScheduleCnt = 4;
        }
    }
    oldBitSet = newBitSet;

    *div = clocks >> 8;
}

void Timer::reset_div() {
    *div = 0;
    clocks = 0;
}

void Timer::write_tima(u8 newTima) {
    if (timaScheduleCnt > 0) {
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

static constexpr u8 freqList[4] = {9, 3, 5, 7};
void Timer::write_tac(u8 newTac) {
    bitFreq = freqList[newTac & 0x3];
    this->enabled = (newTac & 0x4) != 0;
}
