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

    switch (rom[0x0147]) {
        case 0x00:
            cartridge = std::make_unique<ROMOnly>(&rom[0]);
            break;
        case 0x01:
            cartridge = std::make_unique<MBC1>(&rom[0]);
            break;
        case 0x02:
            cartridge = std::make_unique<MBC1>(&rom[0]);
            cartridge->setHasRam(true);
            break;
        case 0x03:
            cartridge = std::make_unique<MBC1>(&rom[0]);
            cartridge->setHasRam(true);
            cartridge->setHasBattery(true);
            break;
        default:
            fatal("Unimplemented cartridge type: %d\n", rom[0x0147]);
    }
}

void Memory::print_cartridge_info() { cartridge->print_cartridge_info(); }

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
            // printf("%c", val);
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