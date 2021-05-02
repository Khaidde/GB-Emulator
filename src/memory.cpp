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
    /*
    if (address >= 0x104 && address < 0x8000) {
        static const uint8_t NINTENDO_LOGO[16 * 3] = {
            0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
            0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
            0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
        };
        // return NINTENDO_LOGO[address - 0x104];
    }
    */
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
    switch (address) {
        case JOYP_REG:
            mem[address] = 0xC0 | (val & 0x30) | (mem[address] & 0x0F);
            break;
        case SC_REG:
            if (val == 0x81) printf("%c", read(SB_REG));
            break;
        case DIV_REG:
            timer->reset_div();
            break;
        case TAC_REG:
            timer->set_enable((val >> 2) & 0x1);
            timer->set_frequency(val & 0x3);
            mem[address] = 0xF8 | (val & 0x7);
            break;
        case DMA_REG: {
            u16 start = val << 8;
            for (int i = 0; i < 0xA0; i++) {
                mem[0xFE00 + i] = mem[start + i];
            }
        } break;
        case IF_REG:
            mem[address] = 0xE0 | (val & 0x1F);
            break;
        default:
            mem[address] = val;
    }
}

void Memory::inc(u16 address) { mem[address]++; }