#include "debugger.hpp"

void Debugger::init(CPU* cpu, Memory* memory) {
    this->cpu = cpu;
    this->ppu = memory->ppu;
    this->memory = memory;

    stepCnt = 0;
    pause = false;
}

void Debugger::print_reg(u16 address, const char* name) {
    printf("\t%2.4x: %02x (%s)\n", address, memory->read(address), name);
}

static constexpr const char* INSTR_DISASSEMBLY[256] = {
    "NOP",
    "LD BC, 2",
    "LD (BC), A",
    "INC BC",
    "INC B",
    "DEC B",
    "LD B, 1",
    "RLCA",
    "LD (2), SP",
    "ADD HL, BC",
    "LD A, (BC)",
    "DEC BC",
    "INC C",
    "DEC C",
    "LD C, 1",
    "RRCA",  // 0x
    "STOP",
    "LD DE, 2",
    "LD (DE), A",
    "INC DE",
    "INC D",
    "DEC D",
    "LD D, 1",
    "RLA",
    "JR 1",
    "ADD HL, DE",
    "LD A, (DE)",
    "DEC DE",
    "INC E",
    "DEC E",
    "LD E, 1",
    "RRA",  // 1x
    "JR NZ, 1",
    "LD HL, 2",
    "LD (HL+), A",
    "INC HL",
    "INC H",
    "DEC H",
    "LD H, 1",
    "DAA",
    "JR Z, 1",
    "ADD HL, HL",
    "LD A, (HL+)",
    "DEC HL",
    "INC L",
    "DEC L",
    "LD L, 1",
    "CPL",  // 2x
    "JR NC, 1",
    "LD SP, 2",
    "LD (HL-), A",
    "INC SP",
    "INC (HL)",
    "DEC (HL)",
    "LD (HL), 1",
    "SCF",
    "JR C, 1",
    "ADD HL, SP",
    "LD A, (HL-)",
    "DEC SP",
    "INC A",
    "DEC A",
    "LD A, 1",
    "CCF",  // 3x
    "LD B, B",
    "LD B, C",
    "LD B, D",
    "LD B, E",
    "LD B, H",
    "LD B, L",
    "LD B, (HL)",
    "LD B, A",
    "LD C, B",
    "LD C, C",
    "LD C, D",
    "LD C, E",
    "LD C, H",
    "LD C, L",
    "LD C, (HL)",
    "LD C, A",  // 4x
    "LD D, B",
    "LD D, C",
    "LD D, D",
    "LD D, E",
    "LD D, H",
    "LD D, L",
    "LD D, (HL)",
    "LD D, A",
    "LD E, B",
    "LD E, C",
    "LD E, D",
    "LD E, E",
    "LD E, H",
    "LD E, L",
    "LD E, (HL)",
    "LD E, A",  // 5x
    "LD H, B",
    "LD H, C",
    "LD H, D",
    "LD H, E",
    "LD H, H",
    "LD H, L",
    "LD H, (HL)",
    "LD H, A",
    "LD L, B",
    "LD L, C",
    "LD L, D",
    "LD L, E",
    "LD L, H",
    "LD L, L",
    "LD L, (HL)",
    "LD L, A",  // 6x
    "LD (HL), B",
    "LD (HL), C",
    "LD (HL), D",
    "LD (HL), E",
    "LD (HL), H",
    "LD (HL), L",
    "HALT",
    "LD (HL), A",
    "LD A, B",
    "LD A, C",
    "LD A, D",
    "LD A, E",
    "LD A. H",
    "LD A, L",
    "LD A, (HL)",
    "LD A, A",  // 7x
    "ADD A, B",
    "ADD A, C",
    "ADD A, D",
    "ADD A, E",
    "ADD A, H",
    "ADD A, L",
    "ADD A, (HL)",
    "ADD A, A",
    "ADC A, B",
    "ADC A, C",
    "ADC A, D",
    "ADC A, E",
    "ADC A, H",
    "ADC A, L",
    "ADC A, (HL)",
    "ADC A, A",  // 8x
    "SUB B",
    "SUB C",
    "SUB D",
    "SUB E",
    "SUB H",
    "SUB L",
    "SUB (HL)",
    "SUB A",
    "SBC A, B",
    "SBC A, C",
    "SBC A, D",
    "SBC A, E",
    "SBC A, H",
    "SBC A, L",
    "SBC A, (HL)",
    "SBC A, A",  // 9x
    "AND B",
    "AND C",
    "AND D",
    "AND E",
    "AND H",
    "AND L",
    "AND (HL)",
    "AND A",
    "XOR B",
    "XOR C",
    "XOR D",
    "XOR E",
    "XOR H",
    "XOR L",
    "XOR (HL)",
    "XOR A",  // Ax
    "OR B",
    "OR C",
    "OR D",
    "OR E",
    "OR H",
    "OR L",
    "OR (HL)",
    "OR A",
    "CP B",
    "CP C",
    "CP D",
    "CP E",
    "CP H",
    "CP L",
    "CP (HL)",
    "CP A",  // Bx
    "RET NZ",
    "POP BC",
    "JP NZ, 2",
    "JP 2",
    "CALL NZ, 2",
    "PUSH BC",
    "ADD A, 1",
    "RST 00h",
    "RET Z",
    "RET",
    "JP Z, 2",
    "PRE CB",
    "CALL Z, 2",
    "CALL 2",
    "ADC A, 1",
    "RST 08h",  // Cx
    "RET NC",
    "POP DE",
    "JP NC, 2",
    "UNKNOWN",
    "CALL NC, 2",
    "PUSH DE",
    "SUB 1",
    "RST 10h",
    "RET C",
    "RETI",
    "JP C, 2",
    "UNKNOWN",
    "CALL C, 2",
    "UNKNOWN",
    "SBC A, 1",
    "RST 18h",  // Dx
    "LDH (ff1h), A",
    "POP HL",
    "LD (ff00h + C), A",
    "UNKNOWN",
    "UNKNOWN",
    "PUSH HL",
    "AND 1",
    "RST 20h",
    "ADD SP, 1",
    "JP (HL)",
    "LD (2), A",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "XOR 1",
    "RST 28h",  // Ex
    "LDH A, (ff1h)",
    "POP AF",
    "LD A, (ff00h + C)",
    "DI",
    "UNKNOWN",
    "PUSH AF",
    "OR 1",
    "RST 30h",
    "LD HL, SP+1",
    "LD SP, HL",
    "LD A, (2)",
    "EI",
    "UNKNOWN",
    "UNKNOWN",
    "CP 1",
    "RST 38h",  // Fx
};
std::string Debugger::disassemble(u16 pc) {
    u8 opcode = memory->read(pc);
    u8 byte1 = memory->read(pc + 1);
    u8 byte2 = memory->read(pc + 2);
    std::string diss(INSTR_DISASSEMBLY[opcode]);

    size_t bytePos = diss.find("1");
    if (bytePos != std::string::npos) {
        char byteChar[20];
        sprintf(byteChar, "%02x", byte1);
        diss.replace(bytePos, 1, byteChar);
    }
    size_t wordPos = diss.find("2");
    if (wordPos != std::string::npos) {
        char wordChar[20];
        sprintf(wordChar, "%04x", byte1 | (byte2 << 8));
        diss.replace(wordPos, 2, wordChar);
    }
    return diss;
}

static constexpr u8 PPU_STATE_MAP[] = {
    2, 2, 2, 2, 3, 3, 3, 0, 0, 1, 1, 1, 1, 1, 1,
};
static constexpr struct {
    const char* name;
    u16 avgClockCompletion;
    u16 avgClocks;
} PPU_STATE_INFO[] = {
    {"HBLANK", 80 + 172 + 204, 204},
    {"VBLANK", 456, 456},
    {"OAM", 80, 80},
    {"LCD", 80 + 172, 172},
};
void Debugger::print_info() {
    printf("---------------\n");
    printf(" %04x => %s => %02x\n", cpu->PC, disassemble(cpu->PC).c_str(), memory->read(cpu->PC));

    printf("\tZNHC=%d%d%d%d\n", cpu->check_flag(CPU::Flag::Z), cpu->check_flag(CPU::Flag::N),
           cpu->check_flag(CPU::Flag::H), cpu->check_flag(CPU::Flag::C));
    printf("\tAF=%04x BC=%04x DE=%04x HL=%04x SP=%04x\n", cpu->regs.AF, cpu->regs.BC, cpu->regs.DE,
           cpu->regs.HL, cpu->SP);
    printf("\time=%d if=%02x ie=%02x\n", cpu->ime, memory->read(IOReg::IF_REG),
           memory->read(IOReg::IE_REG));
    printf("\tly=%02x lcdc=%02x stat=%02x\n", memory->read(IOReg::LY_REG),
           memory->read(IOReg::LCDC_REG), memory->read(IOReg::STAT_REG));

    u8 state = PPU_STATE_MAP[(int)ppu->curPPUState];
    auto stateInfo = PPU_STATE_INFO[state];
    printf("\tcc=%d/~%d mode=%s\n", (stateInfo.avgClockCompletion - ppu->lineClocks) / 2,
           stateInfo.avgClocks / 2, stateInfo.name);
    printf("\ttimerClock=%04x\n", memory->timer->cycles * 2);

    // print_reg(IOReg::JOYP_REG, "Joypad");

    // print_reg(IOReg::SB_REG, "Serial Transfer Data");
    // print_reg(IOReg::SC_REG, "Serial Transfer Control");

    print_reg(IOReg::DIV_REG, "Divide");
    print_reg(IOReg::TIMA_REG, "Timer Counter");
    print_reg(IOReg::TMA_REG, "Timer Modulo");
    print_reg(IOReg::TAC_REG, "Timer Control");

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
    if (cpu->regs.B == 3 && cpu->regs.C == 5) {
        if (cpu->regs.D == 8 && cpu->regs.E == 13) {
            if (cpu->regs.H == 21 && cpu->regs.L == 34) {
                return true;
            }
        }
    }
    return false;
}
