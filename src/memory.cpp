#include "memory.hpp"

#include <cstdio>
#include <fstream>
#include <string.h>
#include <vector>

Memory::Memory() {
    reset_cycles();
    restart();
}

void Memory::restart() {
    mem[IOReg::TIMA_REG] = 0x00;
    mem[IOReg::TMA_REG] = 0x00;
    mem[IOReg::TAC_REG] = 0xF8;

    mem[IOReg::LCDC_REG] = 0x91;
    mem[IOReg::STAT_REG] = 0x85;  // ppu mode should be 1 (v_blank)
    mem[IOReg::SCY_REG] = 0x00;
    mem[IOReg::SCX_REG] = 0x00;
    mem[IOReg::LYC_REG] = 0x00;
    mem[IOReg::BGP_REG] = 0xFC;
    mem[IOReg::OBP0_REG] = 0xFF;
    mem[IOReg::OBP1_REG] = 0xFF;

    mem[IOReg::WY_REG] = 0x00;
    mem[IOReg::WX_REG] = 0x00;

    mem[IOReg::IF_REG] = 0xE1;
    mem[IOReg::IE_REG] = 0x00;
}

void Memory::set_debugger(Debugger* debugger) { this->debugger = debugger; }

void Memory::set_peripherals(Input* input, Timer* timer, PPU* ppu) {
    this->input = input;
    this->timer = timer;
    this->ppu = ppu;
}

void Memory::load_cartridge(const char* romPath) {
    std::ifstream file(romPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        fatal("Can't read from file: %s\n", romPath);
    }

    size_t romSize = file.tellg();
    std::vector<u8> rom(romSize);
    file.seekg(0);
    file.read((char*)&rom[0], romSize);
    file.close();

    char title[16];
    int i = 0;
    for (int c = 0x134; c < 0x144; c++) {
        title[i++] = rom[c];
    }
    printf("Title: %s\n", title);

    static constexpr const char* ROM_SIZES[] = {
        "32 KByte (no ROM banking)",
        "64 KByte (4 banks)",
        "128 KByte (8 banks)",
        "256 KByte (16 banks)",
        "512 KByte (32 banks)",
        "1 MByte (64 banks or 63 on MBC1)",
        "2 MByte (128 banks or 127 on MBC1)",
        "4 MByte (256 banks)",
        "8 MByte (512 banks)",
    };
    u8 numRomBanks = 2;
    u8 romSizeKey = rom[0x0148];
    if (romSizeKey <= 0x8) {
        for (int i = 0; i < romSizeKey; i++) {
            numRomBanks *= 2;
        }
        printf("ROM Size: %s\n", ROM_SIZES[romSizeKey]);
    } else if (0x52 <= romSizeKey && romSizeKey <= 0x54) {
        fatal("Unsupported rom size: %d\n", romSizeKey);
    } else {
        fatal("Unknown rom size: %d\n", romSizeKey);
    }

    static constexpr struct {
        const char* desc;
        char numBanks;
    } RAM_SIZES[] = {
        {"None", 0},
        {"2 KBytes", 1},
        {"8 KBytes", 1},
        {"32 KBytes (4 banks of 8KBytes)", 4},
        {"128 KBytes (16 banks of 8KBytes)", 16},
        {"64 KBytes (8 banks of 8KBytes)", 8},
    };
    u8 numRamBanks;
    u8 ramSizeKey = rom[0x0149];
    if (ramSizeKey <= 0x5) {
        if (ramSizeKey == 1) {
            fatal("TODO check that 2KByte ram works correctly\n");
        }
        numRamBanks = RAM_SIZES[ramSizeKey].numBanks;
        printf("RAM Size: %s\n", RAM_SIZES[ramSizeKey].desc);
    } else {
        fatal("Unknown ram size: %d\n", ramSizeKey);
    }

    const char* cartridgeTypeName = nullptr;
    switch (rom[0x0147]) {
        case 0x00:
            cartridgeTypeName = "ROM Only";
            cartridge = std::make_unique<ROMOnly>(&rom[0]);
            break;
        case 0x01:
            cartridgeTypeName = "MBC1";
            cartridge = std::make_unique<MBC1>(&rom[0], numRomBanks, numRamBanks);
            break;
        case 0x02:
            cartridgeTypeName = "MBC1 + ram";
            cartridge = std::make_unique<MBC1>(&rom[0], numRomBanks, numRamBanks);
            cartridge->setHasRam(true);
            break;
        case 0x03:
            cartridgeTypeName = "MBC1 + ram + battery";
            cartridge = std::make_unique<MBC1>(&rom[0], numRomBanks, numRamBanks);
            cartridge->setHasRam(true);
            cartridge->setHasBattery(true);
            break;
        default:
            fatal("Unimplemented cartridge type: %d\n", rom[0x0147]);
            break;
    }
    if (cartridgeTypeName) printf("Cartridge Type: %s\n", cartridgeTypeName);

    printf("Destination Code: %s\n", rom[0x014A] ? "Non-Japanese" : "Japanese");
    printf("Version: %d\n", rom[0x14C]);

    int x = 0;
    for (int i = 0x0134; i <= 0x014C; i++) {
        x = x - rom[i] - 1;
    }
    printf("Checksum: %s\n", (x & 0xF) == rom[0x014D] ? "PASSED" : "FAILED");
}

void Memory::request_interrupt(Interrupt interrupt) { write(IOReg::IF_REG, read(IOReg::IF_REG) | (u8)interrupt); }

u8& Memory::ref(u16 addr) { return mem[addr]; }

u8 Memory::read(u16 addr) {
    if (addr < 0x8000 || (0xA000 <= addr && addr < 0xC000)) {
        return cartridge->read(addr);
    }
    if (0xFEA0 <= addr && addr < 0xFF00) return 0;
    switch (addr) {
        case IOReg::JOYP_REG: {
            bool btnSelect = ~mem[addr] & (1 << 5);
            bool dirSelect = ~mem[addr] & (1 << 4);
            mem[addr] = input->get_key_state(btnSelect, dirSelect);
            break;
        }
        case IOReg::LY_REG:
            return ppu->read_ly();
        default:
            break;
    }
    return mem[addr];
}

void Memory::write(u16 addr, u8 val) {
    if (addr < 0x8000 || (0xA000 <= addr && addr < 0xC000)) {
        cartridge->write(addr, val);
        return;
    }
    if (0xC000 <= addr && addr < 0xDE00) {
        mem[addr] = val;
        mem[addr + 0x2000] = val;
        return;
    }
    switch (addr) {
        case IOReg::JOYP_REG:
            mem[addr] = 0xC0 | (val & 0x30) | (mem[addr] & 0x0F);
            return;
        case IOReg::SB_REG:
            printf("%c", val);
            break;
        case IOReg::SC_REG:
            mem[addr] = (val & (1 << 8)) | 0x7E | (val & 1);
            return;
        case IOReg::DIV_REG:
            timer->reset_div();
            return;
        case IOReg::TAC_REG:
            timer->set_enable((val >> 2) & 0x1);
            timer->set_frequency(val & 0x3);
            mem[addr] = 0xF8 | (val & 0x7);
            return;
        case IOReg::LCDC_REG:
            if ((val & (1 << 7)) == 0) mem[IOReg::LY_REG] = 0;
            break;
        case IOReg::STAT_REG:
            mem[addr] = 0x80 | (val & 0x78) | (mem[addr] & 0x7);
            ppu->update_stat();
            ppu->trigger_stat_intr();
            return;
        case IOReg::LYC_REG:
            mem[addr] = val;
            ppu->update_stat();
            ppu->trigger_stat_intr();
            return;
        case IOReg::DMA_REG: {
            if (0xFE <= val && val <= 0xFF) {
                printf("TODO illegal DMA source value\n");
                val = mem[addr];
            }
            u16 start = val << 8;
            for (int i = 0; i < 0xA0; i++) {
                mem[0xFE00 + i] = read(start + i);
            }
        } break;
        case IOReg::IF_REG:
            mem[addr] = 0xE0 | (val & 0x1F);
            return;
    }
    mem[addr] = val;
}

void Memory::schedule_read(u8* dest, u16 addr, u8 cycle) {
    MemoryOp readOp;
    readOp.cycle = cycle;
    readOp.isWrite = false;
    readOp.addr = addr;
    readOp.readDest = dest;
    scheduledMemoryOps.push_tail(std::move(readOp));
}

void Memory::schedule_write(u16 addr, u8* val, u8 cycle) {
    MemoryOp writeOp;
    writeOp.cycle = cycle;
    writeOp.isWrite = true;
    writeOp.addr = addr;
    writeOp.writeVal = val;
    scheduledMemoryOps.push_tail(std::move(writeOp));
}

void Memory::emulate_cycle() {
    cycleCnt++;
    if (scheduledMemoryOps.size > 0) {
        if (scheduledMemoryOps.head().cycle == cycleCnt) {
            MemoryOp memOp = scheduledMemoryOps.pop_head();
            if (memOp.isWrite) {
                write(memOp.addr, *memOp.writeVal);
            } else {
                *memOp.readDest = read(memOp.addr);
            }
        }
    }
}

void Memory::reset_cycles() { cycleCnt = 0; }