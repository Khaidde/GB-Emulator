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
        clocks = 0x2674;
    } else {
        // 0xABC8 <= clocks <= 0xABCC
        clocks = 0xABC8;
    }
    timaScheduleCnt = 0;

    oldBitSet = false;

    timaWrote = false;
    timaReloaded = false;

    write_tac(0xF8);
}

static bool newBitSet;
void Timer::emulate_clock() {
    clocks++;
    *div = clocks >> 8;

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
    try_trigger_tima();
}

void Timer::try_trigger_tima() {
    bool newBitSet = (clocks >> bitFreq) & enabled;
    if (!newBitSet && oldBitSet) {
        (*tima)++;
        if (*tima == 0) {
            timaScheduleCnt = 4;
        }
    }
    oldBitSet = newBitSet;
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
    try_trigger_tima();
}
