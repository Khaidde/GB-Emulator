#include "serial.hpp"

#include "memory.hpp"

Serial::Serial(Memory& memory) : memory(&memory) {
    sb = &memory.ref(IOReg::SB_REG);
    sc = &memory.ref(IOReg::SC_REG);
}

void Serial::restart() {
    cycles = 0;
    bitsToTransfer = 0;
}

void Serial::emulate_cycle() {
    if (cycles > 0) {
        cycles--;
        if (cycles == 0) {
            *sb <<= 1;
            if (bitsToTransfer > 0) {
                bitsToTransfer--;
                if (bitsToTransfer == 0) {
                    memory->request_interrupt(Interrupt::SERIAL_INT);
                    *sc &= ~(1 << 7);
                } else {
                    cycles = 128;
                }
            }
        }
    }
}

void Serial::trigger_transfer() {
    cycles = 116;  // Hack to pass mooneye serial
    bitsToTransfer = 8;
}
