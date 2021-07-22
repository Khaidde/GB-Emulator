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
constexpr u8 CGB_BOOT_IO[] = {
    0xFF, 0x00, 0x7E, 0xFF, 0x2F, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1,  // FF00
    0x80, 0xBF, 0xF3, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF,  // FF10
    0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // FF20
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,  // FF30
    0x91, 0x81, 0x00, 0x00, 0x90, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x7E, 0xFF, 0xFE,  // FF40
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // FF50
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xFF, 0xC1, 0x8A, 0xFE, 0xFF, 0xFF, 0xFF,  // FF60
    0xF8,
};
// clang-format on
}  // namespace

void Memory::restart() {
    curWramBank = &wramBanks[1];

    if (is_CGB()) {
        for (int i = 0xFF00; i <= 0xFF70; i++) {
            mem[i] = CGB_BOOT_IO[i - 0xFF00];
        }
        for (int i = 0xFF71; i < 0xFF80; i++) {
            mem[i] = 0xFF;
        }
    } else {
        for (int i = 0xFF00; i < 0xFF50; i++) {
            mem[i] = DMG_BOOT_IO[i - 0xFF00];
        }
        for (int i = 0xFF50; i < 0xFF80; i++) {
            mem[i] = 0xFF;
        }
    }
    mem[IOReg::IE_REG] = 0x00;

    dmaInProgress = false;
    scheduleDma = false;
    dmaCycleCnt = 0;

    elapsedCycles = 0;
}

void Memory::request_interrupt(Interrupt interrupt) { mem[IOReg::IF_REG] |= (u8)interrupt; }

u8& Memory::ref(u16 addr) { return mem[addr]; }

u8 Memory::read(u16 addr) {
    if (addr < 0x8000) {
        return cartridge->read_rom(addr);
    }
    if (addr < 0xA000) {
        return ppu->read_vram(addr);
    }
    if (addr < 0xC000) {
        return cartridge->read_ram(addr);
    }
    if (addr < 0xD000) {
        return wramBanks[0][addr - 0xC000];
    }
    if (addr < 0xE000) {
        return (*curWramBank)[addr - 0xD000];
    }
    if (addr < 0xFE00) {
        return read(addr - 0x2000);
    }
    if (addr < 0xFF00) {
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
    if (0x8000 <= addr && addr < 0xA000) {
        ppu->write_vram(addr, val);
        return;
    }
    if (0xC000 <= addr && addr < 0xD000) {
        wramBanks[0][addr - 0xC000] = val;
        return;
    }
    if (0xD000 <= addr && addr < 0xE000) {
        (*curWramBank)[addr - 0xD000] = val;
        return;
    }
    if (0xE000 <= addr && addr < 0xFE00) {
        write(addr - 0x2000, val);
        return;
    }
    if (0xFE00 <= addr && addr < 0xFF00) {
        if (dmaInProgress) {
            if (addr < 0xFEA0) {
                return;  // TODO assuming that oam writes have no effect during dma
            } else {
                fatal("TODO write to addr=%04x with undefined behavior", addr);
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
    if (!is_CGB() && 0xFF4C <= addr && addr <= 0xFF7F) {
        return;
    }
    switch (addr) {
        case IOReg::JOYP_REG:
            mem[addr] = 0xC0 | (val & 0x30) | (mem[addr] & 0x0F);
            break;
        case IOReg::SB_REG:
#if DEBUG && LOG
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
        case IOReg::TIMA_REG:
            timer->write_tima(val);
            break;
        case IOReg::TMA_REG:
            timer->write_tma(val);
            break;
        case IOReg::TAC_REG:
            timer->write_tac(val);
            mem[addr] = 0xF8 | val;
            break;
        case IOReg::IF_REG:
            mem[addr] = 0xE0 | (val & 0x1F);
            break;
        case IOReg::LCDC_REG:
        case IOReg::STAT_REG:
        case IOReg::LYC_REG:
        case IOReg::BGPI_REG:
        case IOReg::BGPD_REG:
        case IOReg::OBPI_REG:
        case IOReg::OBPD_REG:
            ppu->write_register(addr, val);
            break;
        case IOReg::DMA_REG:
            // TODO verify this. According to docs, val >= 0xE0 is undefined behavior
            if (val >= 0xE0) {
                val &= ~0x20;
            }
            scheduleDma = true;  // DMA idles for 1 cycle before starting
            dmaStartAddr = val << 8;
            mem[addr] = val;
            break;
        case IOReg::KEY1_REG: {
            fatal("TODO Unimplemented speed switch register\n");
        }
        case IOReg::VBK_REG:
            if (is_CGB()) {
                ppu->write_vbk(val);
                mem[addr] = 0xFE | val;
            }
            break;
        case IOReg::HDMA1_REG:
        case IOReg::HDMA2_REG:
        case IOReg::HDMA3_REG:
        case IOReg::HDMA4_REG:
        case IOReg::HDMA5_REG: {
            fatal("TODO Unimplemented hdma\n");
        }
        case IOReg::RP_REG: {
            fatal("TODO Unimplemented infared communications port\n");
        }
        case IOReg::SVBK_REG:
            if (is_CGB()) {
                u8 bank = (val & 0x7);
                curWramBank = &wramBanks[bank == 0 ? 1 : bank];
                mem[addr] = 0xF8 | val;
            }
            break;
        default:
            mem[addr] = val;
    }
}

void Memory::emulate_dma_cycle() {
    if (dmaCycleCnt == 0) {
        dmaInProgress = false;
    }
    if (dmaCycleCnt > 0) {
        u8 i = 160 - dmaCycleCnt;
        if (i == 0) {
            dmaInProgress = true;
        }
        mem[0xFE00 + i] = read(dmaStartAddr + i);
        dmaCycleCnt--;
    }
    if (scheduleDma) {
        dmaCycleCnt = 160;  // DMA takes 160 cycles
        scheduleDma = false;
    }
}

int Memory::get_elapsed_cycles() { return elapsedCycles; }

void Memory::reset_elapsed_cycles() { elapsedCycles -= PPU::TOTAL_CLOCKS; }

void Memory::sleep_cycle() {
    emulate_dma_cycle();
    for (int i = 0; i < 4; i++) {
        timer->emulate_clock();
        apu->emulate_clock();
        ppu->emulate_clock();
    }
    elapsedCycles += 4;
}
