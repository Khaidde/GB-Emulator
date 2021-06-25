#include "debugger.hpp"

void Debugger::init(CPU* cpu, PPU* ppu, Memory* memory) {
    this->cpu = cpu;
    this->ppu = ppu;
    this->memory = memory;

    stepCnt = 0;
    pause = false;
}

void Debugger::print_reg(u16 address, const char* name) {
    printf("\t%2.4x: %02x (%s)\n", address, memory->read(address), name);
}

void Debugger::print_info() {
    printf(" => %04x (PC)\n", cpu->PC);
    printf("\tZNHC=%d%d%d%d\n", cpu->check_flag(CPU::Z_FLAG), cpu->check_flag(CPU::N_FLAG),
           cpu->check_flag(CPU::H_FLAG), cpu->check_flag(CPU::C_FLAG));
    printf("\tAF=%04x BC=%04x DE=%04x HL=%04x SP=%04x\n", cpu->AF.pair, cpu->BC.pair, cpu->DE.pair,
           cpu->HL.pair, cpu->SP);
    printf("\time=%d if=%02x ie=%02x\n", cpu->ime, memory->read(IOReg::IF_REG),
           memory->read(IOReg::IE_REG));
    printf("\tly=%02x lcdc=%02x stat=%02x\n", memory->read(IOReg::LY_REG),
           memory->read(IOReg::LCDC_REG), memory->read(IOReg::STAT_REG));

    printf("cc=%d (%d)\n", (ppu->clockCnt + 4) / 2, ppu->curPPUState);

    // print_reg(IOReg::JOYP_REG, "Joypad");

    // print_reg(IOReg::SB_REG, "Serial Transfer Data");
    // print_reg(IOReg::SC_REG, "Serial Transfer Control");

    // print_reg(IOReg::DIV_REG, "Divide");
    // print_reg(IOReg::TIMA_REG, "Timer Counter");
    // print_reg(IOReg::TMA_REG, "Timer Modulo");
    // print_reg(IOReg::TAC_REG, "Timer Control");

    // print_reg(IOReg::SCY_REG, "Y-Scoll");
    // print_reg(IOReg::SCX_REG, "X-Scoll");
    // print_reg(IOReg::BGP_REG, "Background Palette");
    // print_reg(IOReg::OBP0_REG, "Object Palette 0");
    // print_reg(IOReg::OBP1_REG, "Object Palette 1");
    // print_reg(IOReg::WY_REG, "Window Y");
    // print_reg(IOReg::WX_REG, "Window X");
}

bool Debugger::can_step() {
    if (stepCnt > 0) {
        stepCnt--;
        return true;
    }
    return false;
}

bool Debugger::check_fib_in_registers() {
    if (cpu->BC.hi == 3 && cpu->BC.lo == 5) {
        if (cpu->DE.hi == 8 && cpu->DE.lo == 13) {
            if (cpu->HL.hi == 21 && cpu->HL.lo == 34) {
                return true;
            }
        }
    }
    return false;
}
