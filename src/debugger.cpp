#include "debugger.hpp"

void Debugger::init(CPU* cpu, Memory* memory) {
    this->cpu = cpu;
    this->memory = memory;

    stepCnt = 0;
    pause = false;
}

void Debugger::update_instr(u16 opPC) {
    this->opPC = opPC;
    instrByteLen = 0;
}

void Debugger::print_reg(u16 address, const char* name) {
    printf("\t%2.4x: %02x (%s)\n", address, memory->read(address), name);
}

void Debugger::print_instr() {
    printf("[%2.4x] %02x ", opPC, memory->read(opPC));
    for (int i = instrByteLen - 1; i > 0; i--) {
        printf("%02x", memory->read(opPC + i));
    }
    printf("\n");

    printf("\tZNHC=%d%d%d%d\n", cpu->check_flag(CPU::Z_FLAG), cpu->check_flag(CPU::N_FLAG),
           cpu->check_flag(CPU::H_FLAG), cpu->check_flag(CPU::C_FLAG));
    printf("\tAF=%04x BC=%04x DE=%04x HL=%04x\n", cpu->AF.pair, cpu->BC.pair, cpu->DE.pair, cpu->HL.pair);
    printf("\tSP=%04x PC=%04x\n", cpu->SP, cpu->PC);
    printf("\time=%d if=%02x ie=%02x\n", cpu->ime, memory->read(Memory::IF_REG), memory->read(Memory::IE_REG));
    printf("\tly=%02x lcdc=%02x stat=%02x\n", memory->read(Memory::LY_REG), memory->read(Memory::LCDC_REG),
           memory->read(Memory::STAT_REG));

    // print_reg(Memory::JOYP_REG, "Joypad");

    // print_reg(Memory::SB_REG, "Serial Transfer Data");
    // print_reg(Memory::SC_REG, "Serial Transfer Control");

    print_reg(Memory::DIV_REG, "Divide");
    print_reg(Memory::TIMA_REG, "Timer Counter");
    print_reg(Memory::TMA_REG, "Timer Modulo");
    print_reg(Memory::TAC_REG, "Timer Control");

    print_reg(Memory::LYC_REG, "lyc");
    /*
    print_reg(Memory::SCY_REG, "Y-Scoll");
    print_reg(Memory::SCX_REG, "X-Scoll");
    print_reg(Memory::BGP_REG, "Background Palette");
    print_reg(Memory::OBP0_REG, "Object Palette 0");
    print_reg(Memory::OBP1_REG, "Object Palette 1");
    print_reg(Memory::WY_REG, "Window Y");
    print_reg(Memory::WX_REG, "Window X");
    */
}

void Debugger::handle_function_key(SDL_Event e) {
    if (e.key.keysym.sym == SDLK_F7) {
        stepCnt++;
    }
    if (e.key.keysym.sym == SDLK_F9) {
        continue_exec();
    }
}

bool Debugger::step() {
    if (stepCnt > 0) {
        stepCnt--;
        return true;
    }
    return false;
}