#include "memory.hpp"

namespace {
// clang-format off
constexpr u8 DMG_BOOT_IO[] = {
    0xCF, 0x00, 0x7E, 0xFF, 0xAB, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1,  // FF00
    0x80, 0xBF, 0xF3, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF,  // FF10
    0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // FF20
    0x92, 0xFF, 0x20, 0xEA, 0x86, 0x7D, 0x14, 0x7E, 0x96, 0x7F, 0x00, 0xB9, 0x2C, 0x7A, 0x86, 0x3A,  // FF30
    0x91, 0x85, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFC, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  // FF40
};
// clang-format on

}  // namespace

void Memory::restart() {
    for (int i = 0xFF00; i < 0xFF50; i++) {
        mem[i] = DMG_BOOT_IO[i - 0xFF00];
    }
    for (int i = 0xFF50; i < 0xFF80; i++) {
        mem[i] = 0xFF;
    }
    mem[IOReg::IE_REG] = 0x00;

    dmaInProgress = false;
    scheduleDma = false;
    dmaCycleCnt = 0;
    scheduledMemoryOps.clear();
    reset_cycles();
}

void Memory::request_interrupt(Interrupt interrupt) {
    write(IOReg::IF_REG, read(IOReg::IF_REG) | (u8)interrupt);
}

u8& Memory::ref(u16 addr) { return mem[addr]; }

u8 Memory::read(u16 addr) {
    if (addr < 0x8000) {
        return cartridge->read_rom(addr);
    }
    if (addr < 0xA000) {
        if (ppu->is_vram_read_blocked()) {
            return 0xFF;
        }
        return mem[addr];
    }
    if (addr < 0xC000) {
        return cartridge->read_ram(addr);
    }
    if (0xFE00 <= addr && addr < 0xFF00) {
        if (dmaInProgress) {
            return addr < 0xFEA0 ? 0xFF : 0x00;
        }
        if (ppu->is_oam_read_blocked()) {
            return 0xFF;
        }
        return mem[addr];
    }
    if (0xFF10 <= addr && addr <= 0xFF26) {
        return apu->read_register(mem[addr], addr & 0xFF);
    }
    if (addr == IOReg::JOYP_REG) {
        return input->get_key_state(mem[addr]);
    }
    return mem[addr];
}

void Memory::write(u16 addr, u8 val) {
    if (addr < 0x8000 || (0xA000 <= addr && addr < 0xC000)) {
        cartridge->write(addr, val);
        return;
    }
    if (ppu->is_vram_write_blocked() && 0x8000 <= addr && addr < 0xA000) {
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
        if (ppu->is_oam_write_blocked()) {
            return;
        }
    }
    if (0xFF03 == addr) {
        return;
    }
    if (0xFF08 <= addr && addr <= 0xFF0E) {
        return;
    }
    if (0xFF10 <= addr && addr <= 0xFF26) {
        apu->write_register(addr & 0xFF, val);
        mem[addr] = val;
        return;
    }
    if (0xFF27 <= addr && addr <= 0xFF30) {
        return;
    }
    if (addr == IOReg::LCDC_REG || addr == IOReg::STAT_REG || addr == IOReg::LYC_REG) {
        ppu->write_register(addr, val);
        return;
    }
    if (0xFF4C <= addr && addr <= 0xFF7F) {
        return;
    }
    switch (addr) {
        case IOReg::JOYP_REG:
            mem[addr] = 0xC0 | (val & 0x30) | (mem[addr] & 0x0F);
            break;
        case IOReg::SB_REG:
#if PLAYABLE
            printf("%c", val);
#endif
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
        case IOReg::DMA_REG:
            if (val >= 0xFE) {
                fatal("TODO illegal DMA source value: %02x\n", val);
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

void Memory::schedule_read(u16 addr, u8* dest, u8 cycle) {
    scheduledMemoryOps.push_tail({cycle, addr, false, {dest}});
}

void Memory::schedule_write(u16 addr, u8* val, u8 cycle) {
    scheduledMemoryOps.push_tail({cycle, addr, true, {val}});
}

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
