#include "memory.hpp"

#include <cstdio>
#include <string.h>

void Memory::init(Debugger* debugger) {
    this->debugger = debugger;
    restart();
}
void Memory::restart() {
    for (int i = 0x8000; i < MEMORY_SIZE; i++) {
        mem[i] = 0;
    }

    mem[TIMA_REG] = 0x00;
    mem[TMA_REG] = 0x00;
    mem[TAC_REG] = 0xF8;

    mem[LCDC_REG] = 0x91;
    mem[SCY_REG] = 0x00;
    mem[SCX_REG] = 0x00;
    mem[LYC_REG] = 0x00;
    mem[BGP_REG] = 0xFC;
    mem[OBP0_REG] = 0xFF;
    mem[OBP1_REG] = 0xFF;

    mem[WY_REG] = 0x00;
    mem[WX_REG] = 0x00;

    mem[IF_REG] = 0xE0;
    mem[IE_REG] = 0x00;
}

void Memory::load_peripherals(Cartridge* cartridge, Input* input, Timer* timer) {
    this->cartridge = cartridge;
    for (int i = 0; i < Cartridge::CARTRIDGE_SIZE; i++) {
        mem[i] = cartridge->rom[i];
    }
    this->input = input;
    this->timer = timer;
}

void Memory::request_interrupt(Interrupt interrupt) { write(IF_REG, read(IF_REG) | (u8)interrupt); }

u8 Memory::read(u16 address) {
    if (0xFEA0 <= address && address < 0xFF00) {
        return 0;
    }
    switch (address) {
        case JOYP_REG: {
            bool btnSelect = ~mem[address] & (1 << 5);
            bool dirSelect = ~mem[address] & (1 << 4);
            mem[address] = input->get_key_state(btnSelect, dirSelect);
            break;
        }
        default:
            break;
    }
    return mem[address];
}

void Memory::write(u16 address, u8 val) {
    if (address < 0x8000) {
        // TODO deal with cartridges and switching
        return;
    }
    if (0xC000 <= address && address < 0xDE00) {
        mem[address] = val;
        mem[address + 0x2000] = val;
        return;
    }
    switch (address) {
        case JOYP_REG:
            mem[address] = 0xC0 | (val & 0x30) | (mem[address] & 0x0F);
            return;
        case DIV_REG:
            timer->reset_div();
            return;
        case TAC_REG:
            timer->set_enable((val >> 2) & 0x1);
            timer->set_frequency(val & 0x3);
            mem[address] = 0xF8 | (val & 0x7);
            return;
        case LCDC_REG:
            if ((val & (1 << 7)) == 0) mem[LY_REG] = 0;
            break;
        case STAT_REG:
            if (val != 0) {
                printf("TODO stat interrupts not implemented yet! stat=%02x", val);
            }
            break;
        case DMA_REG: {
            if (0xFE <= val && val <= 0xFF) {
                printf("TODO illegal DMA source value");
                val = mem[address];
            }
            u16 start = val << 8;
            for (int i = 0; i < 0xA0; i++) {
                mem[0xFE00 + i] = read(start + i);
            }
        } break;
        case IF_REG:
            mem[address] = 0xE0 | (val & 0x1F);
            return;
    }
    mem[address] = val;
}

void Memory::inc(u16 address) { mem[address]++; }