#include "memory.hpp"

#include <cstdio>
#include <string.h>

void Memory::startup() {
    memset(mem, 0, MEMORY_SIZE);
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (mem[i] != 0) printf("%02x\n", i);
    }

    write(LCDC_REG, 0x91);
    write(SCY_REG, 0x00);
    write(SCX_REG, 0x00);
    write(LYC_REG, 0x00);
    write(BGP_REG, 0xFC);

    write(WY_REG, 0x00);
    write(WX_REG, 0x00);

    write(IF_REG, 0xE0);
    write(IE_REG, 0x00);
}
void Memory::load_cartridge(Cartridge* cartridge) {
    this->cartridge = cartridge;
    for (int i = 0; i < Cartridge::CARTRIDGE_SIZE; i++) {
        if (i < 0x100) {
            mem[i] = cartridge->rom[i];
        } else {
            mem[i] = 0;
        }
    }

    // static u8 test[16] = {0xFF, 0x00, 0xFF, 0x7E, 0x85, 0x81, 0x89, 0x83,
    //   0x93, 0x85, 0xA5, 0x8B, 0xC9, 0x97, 0x7E, 0xFF};
    // static u8 test2[16] = {0xFF, 0x00, 0x7E, 0x7E, 0x78, 0x78, 0x78, 0x78,
    //    0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0xFF, 0x00};

    int c = 0;
    for (int i = 0x8000; i < 0x9800; i++) {
        if ((i / 16) % 2 == 0) {
            // mem[i] = test[c++];
        }
        // mem[i] = data[c++];
        c %= 16 * 3;
    }
}

void Memory::request_interrupt(Interrupt interrupt) { write(IF_REG, read(IF_REG) | (u8)interrupt); }

u8 Memory::read(u16 address) { return mem[address]; }

void Memory::write(u16 address, u8 val) {
    if (address >= 0x8000 && address < 0x9FFF) {
        // printf("%02x -> %02x\n", address, val);
    }
    mem[address] = val;
}

void Memory::inc(u16 address) { mem[address]++; }
