#include "memory.hpp"

#include <fstream>
#include <vector>

Memory::Memory() {
    dmaInProgress = false;
    scheduleDma = false;
    dmaCycleCnt = 0;
    scheduledMemoryOps.clear();
    reset_cycles();
}

void Memory::restart() {
    mem[IOReg::SB_REG] = 0x00;
    mem[IOReg::SC_REG] = 0x7E;

    mem[IOReg::IF_REG] = 0xE1;
    mem[IOReg::IE_REG] = 0x00;
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
    int t = 0;
    for (int c = 0x134; c < 0x143; c++) {
        title[t++] = rom[c];
    }
    printf("Title: %s\n", title);

    /*
     80h - Game supports CGB functions, but works on old gameboys also.
     C0h - Game works on CGB only (physically the same as 80h).
     */
    if (rom[0x143] == 0x80) {
        printf("CGB Flag: also works on old gameboys\n");
    } else if (rom[0x143] == 0xC0) {
        printf("CGB Flag: only works on CGB\n");
    } else {
        printf("CGB Flag: %02x\n", rom[0x143]);
    }

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
    if (ppu->is_vram_blocked() && 0x8000 <= addr && addr < 0xA000) {
        // printf("Attempt to read vram during mode 3\n");
        return 0xFF;
    }
    if (0xFE00 <= addr && addr < 0xFF00) {
        if (dmaInProgress) {
            return addr < 0xFEA0 ? 0xFF : 0x00;
        }
        if (ppu->is_oam_blocked()) {
            return 0xFF;
        }
        return mem[addr];
    }
    if (0xFF03 == addr) {
        return 0xFF;
    }
    if (0xFF08 <= addr && addr <= 0xFF0E) {
        return 0xFF;
    }
    if (0xFF10 <= addr && addr <= 0xFF26) {
        return apu->read_register(mem[addr], addr & 0xFF);
    }
    if (0xFF27 <= addr && addr <= 0xFF30) {
        return 0xFF;
    }
    if (0xFF4C <= addr && addr <= 0xFF7F) {
        return 0xFF;
    }
    switch (addr) {
        case IOReg::JOYP_REG:
            return input->get_key_state(mem[addr]);
        case IOReg::LY_REG:
            return ppu->read_ly();
        default:
            return mem[addr];
    }
}

void Memory::write(u16 addr, u8 val) {
    if (addr < 0x8000 || (0xA000 <= addr && addr < 0xC000)) {
        cartridge->write(addr, val);
        return;
    }
    if (ppu->is_vram_blocked() && 0x8000 <= addr && addr < 0xA000) {
        // printf("Attempt to write to vram during mode 3\n");
        return;
    }
    if (0xC000 <= addr && addr < 0xDE00) {
        mem[addr] = val;
        mem[addr + 0x2000] = val;
        return;
    }
    if (0xFE00 <= addr && addr < 0xFF00) {
        if (dmaInProgress) {
            if (addr < 0xFEA0) {
                return;  // TODO assuming that oam writes have no effect during dma
            } else {
                fatal("Bad write to addr=%04x", addr);
            }
        }
        if (ppu->is_oam_blocked()) {
            return;
        }
    }
    if (0xFF10 <= addr && addr <= 0xFF26) {
        apu->write_register(addr & 0xFF, val);
        mem[addr] = val;
        return;
    }
    switch (addr) {
        case IOReg::JOYP_REG:
            mem[addr] = 0xC0 | (val & 0x30) | (mem[addr] & 0x0F);
            break;
        case IOReg::SB_REG:
            printf("%c", val);
            mem[addr] = val;
            break;
        case IOReg::SC_REG:
            mem[addr] = (val & (1 << 8)) | 0x7E | (val & 1);
            break;
        case IOReg::DIV_REG:
            timer->reset_div();
            break;
        case IOReg::TAC_REG:
            timer->set_enable((val >> 2) & 0x1);
            timer->set_frequency(val & 0x3);
            mem[addr] = 0xF8 | (val & 0x7);
            break;
        case IOReg::LCDC_REG:
            if ((val & (1 << 7)) == 0) mem[IOReg::LY_REG] = 0;
            mem[addr] = val;
            break;
        case IOReg::STAT_REG:
            mem[addr] = 0x80 | (val & 0x78) | (mem[addr] & 0x7);
            ppu->update_stat();
            ppu->trigger_stat_intr();
            break;
        case IOReg::LYC_REG:
            mem[addr] = val;
            ppu->update_coincidence();
            ppu->trigger_stat_intr();
            break;
        case IOReg::DMA_REG:
            if (0xFE <= val && val <= 0xFF) {
                printf("TODO illegal DMA source value\n");
                val = mem[addr];
            }
            scheduleDma = true;
            dmaStartAddr = val << 8;
            mem[addr] = val;
            break;
        case IOReg::IF_REG:
            mem[addr] = 0xE0 | (val & 0x1F);
            break;
        default:
            mem[addr] = val;
    }
}

void Memory::schedule_read(u16 addr, u8* dest, u8 cycle) { scheduledMemoryOps.push_tail({cycle, addr, false, {dest}}); }

void Memory::schedule_write(u16 addr, u8* val, u8 cycle) { scheduledMemoryOps.push_tail({cycle, addr, true, {val}}); }

void Memory::emulate_dma_cycle() {
    if (dmaCycleCnt > 0) {
        if (dmaCycleCnt <= 160) {
            u8 i = 160 - dmaCycleCnt;
            mem[0xFE00 + i] = read(dmaStartAddr + i);
            if (i == 0) {
                dmaInProgress = true;
            }
        }
        dmaCycleCnt--;
        if (dmaCycleCnt == 0) {
            dmaInProgress = false;
        }
    }
    if (scheduleDma) {
        dmaCycleCnt = 160 + 1;  // DMA takes an extra cycle for initial setup
        scheduleDma = false;
    }
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