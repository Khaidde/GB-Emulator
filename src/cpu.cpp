#include "cpu.hpp"

#include <cstdio>

CPU::CPU(Memory* memory) : memory(memory) {}

void CPU::startup() {
    AF.pair = 0x01B0;
    BC.pair = 0x0013;
    DE.pair = 0x00D8;
    HL.pair = 0x014D;
    SP = 0xFFFE;
    // PC = 0x0100;
    PC = 0;
    ime = false;
    halted = false;
}

static u16 debugPC;

static char memRead;

static u8 cbOp;  // TODO delete
void CPU::tick(u8& cycles) {
    quadCycle = 0;
    branch = false;

    if (PC > 0x100) return;
    if (isUnimp) return;

    if (halted) {
        quadCycle++;
    } else {
        debugPC = PC;
        u8 op = n();
        memRead = 0;
        (this->*opcodes[op])();

        // TODO delete debug info
        bool isCB = false;
        u8* cycleList;
        if (op == 0xCB) {
            isCB = true;
            op = cbOp;
            cycleList = test_cb_cycles;
        } else {
            cycleList = branch ? test_branch_cycles : test_cycles;
        }
        if (quadCycle != cycleList[op]) {
            printf("%02x: %d != expected %d cycles (%02x)\n", PC, quadCycle, cycleList[op], op);
            unimp();
        }
        if (isUnimp) {
            if (isCB) {
                std::printf("CB opcode: ");
            }
            std::printf("Unimplemented opcode for %02x\n", op);
            return;
        } else {
            if (stepMode) {
                printf("---------------------------\n");
                std::printf("[%02.4x] %02x ", debugPC, op);
                for (int i = memRead; i >= 1; i--) {
                    std::printf("%02x", memory->read(debugPC + i));
                }
                std::printf("\n");
                printf("ZNHC=%d%d%d%d\n", check_flag(Z_FLAG), check_flag(N_FLAG), check_flag(H_FLAG),
                       check_flag(C_FLAG));
                printf("AF=%04x BC=%04x\n", AF.pair, BC.pair);
                printf("DE=%04x HL=%04x\n", DE.pair, HL.pair);
                printf("SP: %04x\n", SP);
                printf("PC: %04x\n", PC);
                // printf("FF47 (Background Palette): %02x\n", memory->read(0xFF47));
            } else {
                // if (PC == 0x0034) stepMode = true;
                // if (PC == 0x0040) stepMode = true;
            }
            if (PC == 0x0040) {
                // 0x8190
                for (int i = 0; i < 16; i++) {
                    // printf("%02x", memory->read(0x8190 + i));
                }
                // printf("\n");
                // printf("%02x\n", memory->read(Memory::BGP_REG));
            }
        }
    }

    u8 ifReg = memory->read(Memory::IF_REG);
    u8 interrupts = ifReg & memory->read(Memory::IE_REG) & 0x1F;
    if (interrupts) {
        halted = false;
        if (ime) {
            for (int i = 0; i < 5; i++) {
                if (interrupts & (1 << i)) {
                    printf("Interrupt! %02x from %02x\n", i * 0x8 + 0x40, PC);
                    call(i * 0x8 + 0x40);
                    ime = false;
                    write(Memory::IF_REG, ifReg & ~(1 << i));
                    break;
                }
            }
        }
    }

    cycles = quadCycle;
}

void CPU::unimp() { isUnimp = true; }

u8 CPU::n() {
    memRead++;
    return read(PC++);
}
u16 CPU::nn() {
    memRead += 2;
    return read(PC++) | read(PC++) << 8;
}
void CPU::write(u16 addr, u8 val) {
    quadCycle++;
    memory->write(addr, val);
}
u8 CPU::read(u16 addr) {
    quadCycle++;
    return memory->read(addr);
}
void CPU::set_flag(u8 flag, bool set) { AF.lo = (AF.lo & ~flag) | (flag * set); }
bool CPU::check_flag(u8 flag) { return (AF.lo & flag) != 0; }

// ----- Intructions -----
void CPU::op00() {}            // NOP
void CPU::op10() { unimp(); }  // STOP
void CPU::op76() { unimp(); }  // HALT

// ----- Interrupts -----
void CPU::opF3() { ime = false; }  // DI
void CPU::opFB() {
    // TODO make sure this works correctly
    // unimp();
    ime = true;
}  // EI

// ----- 8-bit -----

// LD (16), A
void CPU::op02() { write(BC.pair, AF.hi); }    // LD (BC), A
void CPU::op12() { write(DE.pair, AF.hi); }    // LD (DE), A
void CPU::op22() { write(HL.pair++, AF.hi); }  // LD (HL+), A
void CPU::op32() { write(HL.pair--, AF.hi); }  // LD (HL-), A

// 8-bit load
void CPU::op06() { BC.hi = n(); }          // LD B, n
void CPU::op16() { DE.hi = n(); }          // LD D, n
void CPU::op26() { HL.hi = n(); }          // LD H, n
void CPU::op36() { write(HL.pair, n()); }  // LD (HL), n
void CPU::op0E() { BC.lo = n(); }          // LD C, n
void CPU::op1E() { DE.lo = n(); }          // LD E, n
void CPU::op2E() { HL.lo = n(); }          // LD L, n
void CPU::op3E() { AF.hi = n(); }          // LD A, n

// LD A, (16)
void CPU::op0A() { AF.hi = read(BC.pair); }    // LD A, (BC)
void CPU::op1A() { AF.hi = read(DE.pair); }    // LD A, (DE)
void CPU::op2A() { AF.hi = read(HL.pair++); }  // LD A, (HL+)
void CPU::op3A() { AF.hi = read(HL.pair--); }  // LD A, (HL-)

// LD B, r2
void CPU::op40() {}                          // LD B, B
void CPU::op41() { BC.hi = BC.lo; }          // LD B, C
void CPU::op42() { BC.hi = DE.hi; }          // LD B, D
void CPU::op43() { BC.hi = DE.lo; }          // LD B, E
void CPU::op44() { BC.hi = HL.hi; }          // LD B, H
void CPU::op45() { BC.hi = HL.lo; }          // LD B, L
void CPU::op46() { BC.hi = read(HL.pair); }  // LD B, (HL)
void CPU::op47() { BC.hi = AF.hi; }          // LD B, A

// LD C, r2
void CPU::op48() { BC.lo = BC.hi; }          // LD C, B
void CPU::op49() {}                          // LD C, C
void CPU::op4A() { BC.lo = DE.hi; }          // LD C, D
void CPU::op4B() { BC.lo = DE.lo; }          // LD C, E
void CPU::op4C() { BC.lo = HL.hi; }          // LD C, H
void CPU::op4D() { BC.lo = HL.lo; }          // LD C, L
void CPU::op4E() { BC.lo = read(HL.pair); }  // LD C, (HL)
void CPU::op4F() { BC.lo = AF.hi; }          // LD C, A

// LD D, r2
void CPU::op50() { DE.hi = BC.hi; }          // LD D, B
void CPU::op51() { DE.hi = BC.lo; }          // LD D, C
void CPU::op52() {}                          // LD D, D
void CPU::op53() { DE.hi = DE.lo; }          // LD D, E
void CPU::op54() { DE.hi = HL.hi; }          // LD D, H
void CPU::op55() { DE.hi = HL.lo; }          // LD D, L
void CPU::op56() { DE.hi = read(HL.pair); }  // LD D, (HL)
void CPU::op57() { DE.hi = AF.hi; }          // LD D, A

// LD E, r2
void CPU::op58() { DE.lo = BC.hi; }          // LD E, B
void CPU::op59() { DE.lo = BC.lo; }          // LD E, C
void CPU::op5A() { DE.lo = DE.hi; }          // LD E, D
void CPU::op5B() {}                          // LD E, E
void CPU::op5C() { DE.lo = HL.hi; }          // LD E, H
void CPU::op5D() { DE.lo = HL.lo; }          // LD E, L
void CPU::op5E() { DE.lo = read(HL.pair); }  // LD E, (HL)
void CPU::op5F() { DE.lo = AF.hi; }          // LD E, A

// LD H, r2
void CPU::op60() { HL.hi = BC.hi; }          // LD H, B
void CPU::op61() { HL.hi = BC.lo; }          // LD H, C
void CPU::op62() { HL.hi = DE.hi; }          // LD H, D
void CPU::op63() { HL.hi = DE.lo; }          // LD H, E
void CPU::op64() {}                          // LD H, H
void CPU::op65() { HL.hi = HL.lo; }          // LD H, L
void CPU::op66() { HL.hi = read(HL.pair); }  // LD H, (HL)
void CPU::op67() { HL.hi = AF.hi; }          // LD H, A

// LD L, r2
void CPU::op68() { HL.lo = BC.hi; }          // LD L, B
void CPU::op69() { HL.lo = BC.lo; }          // LD L, C
void CPU::op6A() { HL.lo = DE.hi; }          // LD L, D
void CPU::op6B() { HL.lo = DE.lo; }          // LD L, E
void CPU::op6C() { HL.lo = HL.hi; }          // LD L, H
void CPU::op6D() {}                          // LD L, L
void CPU::op6E() { HL.lo = read(HL.pair); }  // LD L, (HL)
void CPU::op6F() { HL.lo = AF.hi; }          // LD L, A

// LD (HL), r2
void CPU::op70() { write(HL.pair, BC.hi); }  // LD (HL), B
void CPU::op71() { write(HL.pair, BC.lo); }  // LD (HL), C
void CPU::op72() { write(HL.pair, DE.hi); }  // LD (HL), D
void CPU::op73() { write(HL.pair, DE.lo); }  // LD (HL), E
void CPU::op74() { write(HL.pair, HL.hi); }  // LD (HL), H
void CPU::op75() { write(HL.pair, HL.lo); }  // LD (HL), L
void CPU::op77() { write(HL.pair, AF.hi); }  // LD (HL), A

// LD A, r2
void CPU::op78() { AF.hi = BC.hi; }          // LD A, B
void CPU::op79() { AF.hi = BC.lo; }          // LD A, C
void CPU::op7A() { AF.hi = DE.hi; }          // LD A, D
void CPU::op7B() { AF.hi = DE.lo; }          // LD A, E
void CPU::op7C() { AF.hi = HL.hi; }          // LD A, H
void CPU::op7D() { AF.hi = HL.lo; }          // LD A, L
void CPU::op7E() { AF.hi = read(HL.pair); }  // LD A, (HL)
void CPU::op7F() {}                          // LD A, A

// load $FF00 + n
void CPU::opE0() { write(0xFF00 + n(), AF.hi); }  // LDH ($FF00 + n), A
void CPU::opF0() { AF.hi = read(0xFF00 + n()); }  // LDH A, ($FF00 + n)

// load $FF00 + C
void CPU::opE2() { write(BC.lo + 0xFF00, AF.hi); }  // LD (C), A [A -> $FF00 + C]
void CPU::opF2() { AF.hi = read(BC.lo + 0xFF00); }  // LD A, (C) [$FF00 + C -> A]

void CPU::opEA() { write(nn(), AF.hi); }  // LD (nn), A
void CPU::opFA() { AF.hi = read(nn()); }  // LD A, (nn)

// ----- 16-bit -----

// LD n, nn
void CPU::op01() { BC.pair = nn(); }  // LD BC, nn
void CPU::op11() { DE.pair = nn(); }  // LD DE, nn
void CPU::op21() { HL.pair = nn(); }  // LD HL, nn
void CPU::op31() { SP = nn(); }       // LD SP, nn

// pop
u16 CPU::pop() { return read(SP++) | read(SP++) << 8; }
void CPU::opC1() { BC.pair = pop(); }  // POP BC
void CPU::opD1() { DE.pair = pop(); }  // POP DE
void CPU::opE1() { HL.pair = pop(); }  // POP HL
void CPU::opF1() {
    AF.hi = read(SP++);
    AF.lo = read(SP++) & 0xF0;
}  // POP AF

// push
void CPU::push(u16 reg) {
    quadCycle++;  // Stack push takes an extra cycle
    write(--SP, reg >> 8);
    write(--SP, reg & 0xFF);
}
void CPU::opC5() { push(BC.pair); }  // PUSH BC
void CPU::opD5() { push(DE.pair); }  // PUSH DE
void CPU::opE5() { push(HL.pair); }  // PUSH HL
void CPU::opF5() { push(AF.pair); }  // PUSH AF

void CPU::op08() { write(nn(), SP); }  // LD (nn), SP

void CPU::opF9() { SP = HL.pair; }  // LD SP, HL

// ----- 8-bit ALU -----

// ADD A, n
void CPU::add8_to_A(u8 val) {
    u8 res = AF.hi + val;
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, (AF.hi & 0xF) + (val & 0xF) > 0xF);
    set_flag(C_FLAG, (0xFF - AF.hi) < val);
    AF.hi = res;
}
void CPU::op80() { add8_to_A(BC.hi); }          // ADD A, B
void CPU::op81() { add8_to_A(BC.lo); }          // ADD A, C
void CPU::op82() { add8_to_A(DE.hi); }          // ADD A, D
void CPU::op83() { add8_to_A(DE.lo); }          // ADD A, E
void CPU::op84() { add8_to_A(HL.hi); }          // ADD A, H
void CPU::op85() { add8_to_A(HL.lo); }          // ADD A, L
void CPU::op86() { add8_to_A(read(HL.pair)); }  // ADD A, (HL)
void CPU::op87() { add8_to_A(AF.hi); }          // ADD A, A
void CPU::opC6() { add8_to_A(n()); }            // ADD A, n

// ADC A, n
void CPU::adc8_to_A(u8 val) {
    u8 carry = check_flag(C_FLAG);
    u8 res = AF.hi + val + carry;
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, carry + (AF.hi & 0xF) + (val & 0xF) > 0xF);
    set_flag(C_FLAG, res > 0xFF);
    AF.hi = res;
}
void CPU::op88() { adc8_to_A(BC.hi); }          // ADC A, B
void CPU::op89() { adc8_to_A(BC.lo); }          // ADC A, C
void CPU::op8A() { adc8_to_A(DE.hi); }          // ADC A, D
void CPU::op8B() { adc8_to_A(DE.lo); }          // ADC A, E
void CPU::op8C() { adc8_to_A(HL.hi); }          // ADC A, H
void CPU::op8D() { adc8_to_A(HL.lo); }          // ADC A, L
void CPU::op8E() { adc8_to_A(read(HL.pair)); }  // ADC A, (HL)
void CPU::op8F() { adc8_to_A(AF.hi); }          // ADC A, A
void CPU::opCE() { adc8_to_A(n()); }            // ADC A, n

// SUB A, n
void CPU::sub8_to_A(u8 val) {
    u8 res = AF.hi - val;
    set_flag(Z_FLAG, AF.hi == val);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (AF.hi & 0xF) < (val & 0xF));
    set_flag(C_FLAG, AF.hi < val);
    AF.hi = res;
}
void CPU::op90() { sub8_to_A(BC.hi); }          // SUB B
void CPU::op91() { sub8_to_A(BC.lo); }          // SUB C
void CPU::op92() { sub8_to_A(DE.hi); }          // SUB D
void CPU::op93() { sub8_to_A(DE.lo); }          // SUB E
void CPU::op94() { sub8_to_A(HL.hi); }          // SUB H
void CPU::op95() { sub8_to_A(HL.lo); }          // SUB L
void CPU::op96() { sub8_to_A(read(HL.pair)); }  // SUB (HL)
void CPU::op97() { sub8_to_A(AF.hi); }          // SUB A
void CPU::opD6() { sub8_to_A(n()); }            // SUB n

// SBC A, A
void CPU::sbc8_to_A(u8 val) {
    u8 carry = check_flag(C_FLAG);
    u8 res = AF.hi - (val + carry);
    set_flag(Z_FLAG, AF.hi == val);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (AF.hi & 0xF) - (val & 0xF) - carry < 0);
    set_flag(C_FLAG, AF.hi - val - carry < 0);
    AF.hi = res;
}
void CPU::op98() { sbc8_to_A(BC.hi); }          // SBC A, B
void CPU::op99() { sbc8_to_A(BC.lo); }          // SBC A, C
void CPU::op9A() { sbc8_to_A(DE.hi); }          // SBC A, D
void CPU::op9B() { sbc8_to_A(DE.lo); }          // SBC A, E
void CPU::op9C() { sbc8_to_A(HL.hi); }          // SBC A, H
void CPU::op9D() { sbc8_to_A(HL.lo); }          // SBC A, L
void CPU::op9E() { sbc8_to_A(read(HL.pair)); }  // SBC A, (HL)
void CPU::op9F() { sbc8_to_A(AF.hi); }          // SBC A, A
void CPU::opDE() { sbc8_to_A(n()); }            // SBC A, n

// AND A, n
void CPU::and8_to_A(u8 val) {
    AF.hi &= val;
    set_flag(Z_FLAG, AF.hi == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, true);
    set_flag(C_FLAG, false);
}
void CPU::opA0() { and8_to_A(BC.hi); }          // AND B
void CPU::opA1() { and8_to_A(BC.lo); }          // AND C
void CPU::opA2() { and8_to_A(DE.hi); }          // AND D
void CPU::opA3() { and8_to_A(DE.lo); }          // AND E
void CPU::opA4() { and8_to_A(HL.hi); }          // AND H
void CPU::opA5() { and8_to_A(HL.lo); }          // AND L
void CPU::opA6() { and8_to_A(read(HL.pair)); }  // AND (HL)
void CPU::opA7() { and8_to_A(AF.hi); }          // AND A
void CPU::opE6() { and8_to_A(n()); }            // AND n

// XOR A, n
void CPU::xor8_to_A(u8 val) {
    AF.hi ^= val;
    set_flag(Z_FLAG, AF.hi == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, false);
}
void CPU::opA8() { xor8_to_A(BC.hi); }          // XOR B
void CPU::opA9() { xor8_to_A(BC.lo); }          // XOR C
void CPU::opAA() { xor8_to_A(DE.hi); }          // XOR D
void CPU::opAB() { xor8_to_A(DE.lo); }          // XOR E
void CPU::opAC() { xor8_to_A(HL.hi); }          // XOR H
void CPU::opAD() { xor8_to_A(HL.lo); }          // XOR L
void CPU::opAE() { xor8_to_A(read(HL.pair)); }  // XOR (HL)
void CPU::opAF() { xor8_to_A(AF.hi); }          // XOR A
void CPU::opEE() { xor8_to_A(n()); }            // XOR A, n

// OR A, n
void CPU::or8_to_A(u8 val) {
    AF.hi |= val;
    set_flag(Z_FLAG, AF.hi == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, false);
}
void CPU::opB0() { or8_to_A(BC.hi); }          // OR B
void CPU::opB1() { or8_to_A(BC.lo); }          // OR C
void CPU::opB2() { or8_to_A(DE.hi); }          // OR D
void CPU::opB3() { or8_to_A(DE.lo); }          // OR E
void CPU::opB4() { or8_to_A(HL.hi); }          // OR H
void CPU::opB5() { or8_to_A(HL.lo); }          // OR L
void CPU::opB6() { or8_to_A(read(HL.pair)); }  // OR (HL)
void CPU::opB7() { or8_to_A(AF.hi); }          // OR A
void CPU::opF6() { or8_to_A(n()); }            // OR n

// CP A, n
void CPU::compare8(u8 val) {
    set_flag(Z_FLAG, AF.hi == val);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (AF.hi & 0xF) < (val & 0xF));
    set_flag(C_FLAG, AF.hi < val);
}
void CPU::opB8() { compare8(BC.hi); }          // CP B
void CPU::opB9() { compare8(BC.lo); }          // CP C
void CPU::opBA() { compare8(DE.hi); }          // CP D
void CPU::opBB() { compare8(DE.lo); }          // CP E
void CPU::opBC() { compare8(HL.hi); }          // CP H
void CPU::opBD() { compare8(HL.lo); }          // CP L
void CPU::opBE() { compare8(read(HL.pair)); }  // CP (HL)
void CPU::opBF() { compare8(AF.hi); }          // CP A
void CPU::opFE() { compare8(n()); }            // CP n

// INC n
void CPU::inc8(u8& reg) {
    reg++;
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, reg & 0xF);
}
void CPU::op04() { inc8(BC.hi); }  // INC B
void CPU::op0C() { inc8(BC.lo); }  // INC C
void CPU::op14() { inc8(DE.hi); }  // INC D
void CPU::op1C() { inc8(DE.lo); }  // INC E
void CPU::op24() { inc8(HL.hi); }  // INC H
void CPU::op2C() { inc8(HL.lo); }  // INC L
void CPU::op34() {
    u8 val = read(HL.pair);
    inc8(val);
    write(HL.pair, val);
}  // INC (HL)
void CPU::op3C() { inc8(AF.hi); }  // INC A

// DEC n
void CPU::dec8(u8& reg) {
    reg--;
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (reg & 0xF) == 0xF);
}
void CPU::op05() { dec8(BC.hi); }  // DEC B
void CPU::op0D() { dec8(BC.lo); }  // DEC C
void CPU::op15() { dec8(DE.hi); }  // DEC D
void CPU::op1D() { dec8(DE.lo); }  // DEC E
void CPU::op25() { dec8(HL.hi); }  // DEC H
void CPU::op2D() { dec8(HL.lo); }  // DEC L
void CPU::op35() {
    u8 val = read(HL.pair);
    dec8(val);
    write(HL.pair, val);
}  // DEC (HL)
void CPU::op3D() { dec8(AF.hi); }  // DEC A

// ----- 16-bit ALU -----

// ADD HL, n
void CPU::add16_to_HL(u16 val) {
    u16 res = HL.pair + val;
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, (0xFFF - (HL.pair & 0xFFF)) < (val & 0xFFF));
    set_flag(C_FLAG, (0xFFFF - HL.pair) < val);
    HL.pair = res;
}
void CPU::op09() { add16_to_HL(BC.pair); }  // ADD HL, BC
void CPU::op19() { add16_to_HL(DE.pair); }  // ADD HL, DE
void CPU::op29() { add16_to_HL(HL.pair); }  // ADD HL, HL
void CPU::op39() { add16_to_HL(SP); }       // ADD HL, SP

u16 CPU::addSP_n() {
    u16 stackPtr = SP;
    s8 off = n();
    u16 res = stackPtr + off;

    set_flag(Z_FLAG, false);
    set_flag(N_FLAG, false);

    // TODO Verify that this works:
    // https://stackoverflow.com/questions/5159603/gbz80-how-does-ld-hl-spe-affect-h-and-c-flags
    unimp();
    set_flag(H_FLAG, ((stackPtr ^ off ^ HL.pair) & 0x10) == 0x10);
    set_flag(C_FLAG, ((stackPtr ^ off ^ HL.pair) & 0x100) == 0x100);

    return res;
}
void CPU::opE8() { SP = addSP_n(); }       // ADD SP, n
void CPU::opF8() { HL.pair = addSP_n(); }  // LD HL, SP + n

// INC nn
void CPU::op03() {
    quadCycle++;
    BC.pair++;
}  // INC BC
void CPU::op13() {
    quadCycle++;
    DE.pair++;
}  // INC DE
void CPU::op23() {
    quadCycle++;
    HL.pair++;
}  // INC HL
void CPU::op33() {
    quadCycle++;
    SP++;
}  // INC SP

void CPU::op0B() {
    quadCycle++;
    BC.pair--;
}  // DEC BC
void CPU::op1B() {
    quadCycle++;
    DE.pair--;
}  // DEC DE
void CPU::op2B() {
    quadCycle++;
    HL.pair--;
}  // DEC HL
void CPU::op3B() {
    quadCycle++;
    SP--;
}  // DEC SP

// ----- Misc ALU -----

// Rotate Left
void CPU::op07() { unimp(); }  // RLCA
void CPU::op17() {
    rl8(AF.hi);
    set_flag(Z_FLAG, false);
}  // RLA

// Rotate right
void CPU::op0F() { unimp(); }  // RRCA
void CPU::op1F() { unimp(); }  // RRA

void CPU::op27() { unimp(); }  // DAA
void CPU::op37() { unimp(); }  // SCF

void CPU::op2F() {
    AF.hi = ~AF.hi;
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, true);
}  // CPL
void CPU::op3F() { unimp(); }  // CCF

// ----- Jump -----
void CPU::jump(u16 addr) {
    quadCycle++;
    PC = addr;
}
void CPU::opC3() { jump(nn()); }  // JP nn

void CPU::opC2() {
    u16 addr = nn();
    if (!check_flag(Z_FLAG)) {
        branch = true;
        jump(addr);
    }
}  // JP NZ, nn
void CPU::opD2() {
    u16 addr = nn();
    if (!check_flag(C_FLAG)) {
        branch = true;
        jump(addr);
    }
}  // JP NC, nn
void CPU::opCA() {
    u16 addr = nn();
    if (check_flag(Z_FLAG)) {
        branch = true;
        jump(addr);
    }
}  // JP Z, nn
void CPU::opDA() {
    u16 addr = nn();
    if (check_flag(C_FLAG)) {
        branch = true;
        jump(addr);
    }
}  // JP C, nn

void CPU::opE9() {
    // TODO verify that this works
    unimp();
    jump(HL.pair);
}  // JP (HL)

void CPU::op18() { jump(PC + (s8)n()); }  // JR n
void CPU::op20() {
    s8 val = n();
    if (!check_flag(Z_FLAG)) {
        branch = true;
        jump(PC + val);
    }
}  // JR NZ, n
void CPU::op30() {
    s8 val = n();
    if (!check_flag(C_FLAG)) {
        branch = true;
        jump(PC + val);
    }
}  // JR NC, n
void CPU::op28() {
    s8 val = n();
    if (check_flag(Z_FLAG)) {
        branch = true;
        jump(PC + val);
    }
}  // JR Z, n
void CPU::op38() {
    s8 val = n();
    if (check_flag(C_FLAG)) {
        branch = true;
        jump(PC + val);
    }
}  // JR C, n

// ----- Call -----
void CPU::call(u16 addr) {
    push(PC);
    PC = addr;
}

void CPU::opCD() { call(nn()); }  // CALL
void CPU::opC4() {
    u16 addr = nn();
    if (!check_flag(Z_FLAG)) {
        branch = true;
        call(addr);
    }
}  // CALL NZ, nn
void CPU::opD4() {
    u16 addr = nn();
    if (!check_flag(C_FLAG)) {
        branch = true;
        call(addr);
    }

}  // CALL NC, nn
void CPU::opCC() {
    u16 addr = nn();
    if (check_flag(Z_FLAG)) {
        branch = true;
        call(addr);
    }
}  // CALL Z, nn
void CPU::opDC() {
    u16 addr = nn();
    if (check_flag(C_FLAG)) {
        branch = true;
        call(addr);
    }
}  // CALL C, nn

// ----- Restarts -----
void CPU::opC7() { call(0x00); }  // RST 00H
void CPU::opD7() { call(0x10); }  // RST 10H
void CPU::opE7() { call(0x20); }  // RST 20H
void CPU::opF7() { call(0x30); }  // RST 30H

void CPU::opCF() { call(0x08); }  // RST 08H
void CPU::opDF() { call(0x18); }  // RST 18H
void CPU::opEF() { call(0x28); }  // RST 28H
void CPU::opFF() { call(0x38); }  // RST 38H

// ----- Returns -----
void CPU::opC9() { jump(pop()); }  // RET

void CPU::opC0() {
    quadCycle++;
    if (!check_flag(Z_FLAG)) {
        branch = true;
        jump(pop());
    }
}  // RET NZ
void CPU::opD0() {
    quadCycle++;
    if (!check_flag(C_FLAG)) {
        branch = true;
        jump(pop());
    }
}  // RET NC
void CPU::opC8() {
    quadCycle++;
    if (check_flag(Z_FLAG)) {
        branch = true;
        jump(pop());
    }
}  // RET Z
void CPU::opD8() {
    quadCycle++;
    if (check_flag(C_FLAG)) {
        branch = true;
        jump(pop());
    }
}  // RET C

void CPU::opD9() {
    jump(pop());
    ime = true;
}  // RETI

// ----- Error Opcodes -----
void CPU::opD3() { unimp(); }
void CPU::opE3() { unimp(); }
void CPU::opE4() { unimp(); }
void CPU::opF4() { unimp(); }
void CPU::opDB() { unimp(); }
void CPU::opEB() { unimp(); }
void CPU::opEC() { unimp(); }
void CPU::opFC() { unimp(); }
void CPU::opDD() { unimp(); }
void CPU::opED() { unimp(); }
void CPU::opFD() { unimp(); }

// ----- CB-prefix -----
void CPU::opCB() {
    u8 op = n();
    cbOp = op;
    if (op < 0x40) {
        (this->*opcodesCB[op])();
    } else {
        u8 index = ((op / 0x40) - 1) * 0x8 + (op % 0x8);
        u8 b = (op / 0x8) % 0x8;
        (this->*bopcodesCB[index])(b);
    }
}  // CB-prefix

// SWAP n
void CPU::opCB30() { unimp(); }  // SWAP B
void CPU::opCB31() { unimp(); }  // SWAP C
void CPU::opCB32() { unimp(); }  // SWAP D
void CPU::opCB33() { unimp(); }  // SWAP E
void CPU::opCB34() { unimp(); }  // SWAP H
void CPU::opCB35() { unimp(); }  // SWAP L
void CPU::opCB36() { unimp(); }  // SWAP (HL)
void CPU::opCB37() { unimp(); }  // SWAP A

//  RLC n
void CPU::opCB00() { unimp(); }  // RLC B
void CPU::opCB01() { unimp(); }  // RLC C
void CPU::opCB02() { unimp(); }  // RLC D
void CPU::opCB03() { unimp(); }  // RLC E
void CPU::opCB04() { unimp(); }  // RLC H
void CPU::opCB05() { unimp(); }  // RLC L
void CPU::opCB06() { unimp(); }  // RLC (HL)
void CPU::opCB07() { unimp(); }  // RLC A

//  RRC n
void CPU::opCB08() { unimp(); }  // RRC B
void CPU::opCB09() { unimp(); }  // RRC C
void CPU::opCB0A() { unimp(); }  // RRC D
void CPU::opCB0B() { unimp(); }  // RRC E
void CPU::opCB0C() { unimp(); }  // RRC H
void CPU::opCB0D() { unimp(); }  // RRC L
void CPU::opCB0E() { unimp(); }  // RRC (HL)
void CPU::opCB0F() { unimp(); }  // RRC A

// RL n
void CPU::rl8(u8& reg) {
    u8 res = (reg << 1) | check_flag(C_FLAG);

    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, reg & (1 << 7));

    reg = res;
}
void CPU::opCB10() { rl8(BC.hi); }  // RL B
void CPU::opCB11() { rl8(BC.lo); }  // RL C
void CPU::opCB12() { rl8(DE.hi); }  // RL D
void CPU::opCB13() { rl8(DE.lo); }  // RL E
void CPU::opCB14() { rl8(HL.hi); }  // RL H
void CPU::opCB15() { rl8(HL.lo); }  // RL L
void CPU::opCB16() {
    u8 val = read(HL.pair);
    rl8(val);
    write(HL.pair, val);
}  // RL (HL)
void CPU::opCB17() { rl8(AF.hi); }  // RL A

// RR n
void CPU::opCB18() { unimp(); }  // RR B
void CPU::opCB19() { unimp(); }  // RR C
void CPU::opCB1A() { unimp(); }  // RR D
void CPU::opCB1B() { unimp(); }  // RR E
void CPU::opCB1C() { unimp(); }  // RR H
void CPU::opCB1D() { unimp(); }  // RR L
void CPU::opCB1E() { unimp(); }  // RR (HL)
void CPU::opCB1F() { unimp(); }  // RR A

// SLA n
void CPU::opCB20() { unimp(); }  // SLA B
void CPU::opCB21() { unimp(); }  // SLA C
void CPU::opCB22() { unimp(); }  // SLA D
void CPU::opCB23() { unimp(); }  // SLA E
void CPU::opCB24() { unimp(); }  // SLA H
void CPU::opCB25() { unimp(); }  // SLA L
void CPU::opCB26() { unimp(); }  // SLA (HL)
void CPU::opCB27() { unimp(); }  // SLA A

// SRA n
void CPU::opCB28() { unimp(); }  // SRA B
void CPU::opCB29() { unimp(); }  // SRA C
void CPU::opCB2A() { unimp(); }  // SRA D
void CPU::opCB2B() { unimp(); }  // SRA E
void CPU::opCB2C() { unimp(); }  // SRA H
void CPU::opCB2D() { unimp(); }  // SRA L
void CPU::opCB2E() { unimp(); }  // SRA (HL)
void CPU::opCB2F() { unimp(); }  // SRA A

// SRL n
void CPU::opCB38() { unimp(); }  // SRL B
void CPU::opCB39() { unimp(); }  // SRL C
void CPU::opCB3A() { unimp(); }  // SRL D
void CPU::opCB3B() { unimp(); }  // SRL E
void CPU::opCB3C() { unimp(); }  // SRL H
void CPU::opCB3D() { unimp(); }  // SRL L
void CPU::opCB3E() { unimp(); }  // SRL (HL)
void CPU::opCB3F() { unimp(); }  // SRL A

// BIT b, r
void CPU::bit(u8 b, u8 reg) {
    set_flag(Z_FLAG, (reg & (1 << b)) == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, true);
}
void CPU::opCB_bitB(u8 b) { bit(b, BC.hi); }           // BIT b, B
void CPU::opCB_bitC(u8 b) { bit(b, BC.lo); }           // BIT b, C
void CPU::opCB_bitD(u8 b) { bit(b, DE.hi); }           // BIT b, D
void CPU::opCB_bitE(u8 b) { bit(b, DE.lo); }           // BIT b, E
void CPU::opCB_bitH(u8 b) { bit(b, HL.hi); }           // BIT b, H
void CPU::opCB_bitL(u8 b) { bit(b, HL.lo); }           // BIT b, L
void CPU::opCB_bitHL(u8 b) { bit(b, read(HL.pair)); }  // BIT b, (HL)
void CPU::opCB_bitA(u8 b) { bit(b, AF.hi); }           // BIT b, A

// RES b, r
void CPU::res(u8 b, u8& reg) { reg &= 1 << b; }
void CPU::opCB_resB(u8 b) { res(b, BC.hi); }  // RES b, B
void CPU::opCB_resC(u8 b) { res(b, BC.lo); }  // RES b, C
void CPU::opCB_resD(u8 b) { res(b, DE.hi); }  // RES b, D
void CPU::opCB_resE(u8 b) { res(b, DE.lo); }  // RES b, E
void CPU::opCB_resH(u8 b) { res(b, HL.hi); }  // RES b, H
void CPU::opCB_resL(u8 b) { res(b, HL.lo); }  // RES b, L
void CPU::opCB_resHL(u8 b) {
    u8 val = read(HL.pair);
    res(b, val);
    write(HL.pair, val);
}  // RES b, (HL)
void CPU::opCB_resA(u8 b) { res(b, AF.hi); }  // RES b, A

// SET b, r
void CPU::set(u8 b, u8& reg) { reg |= 1 << b; }
void CPU::opCB_setB(u8 b) { set(b, BC.hi); }  // SET b, B
void CPU::opCB_setC(u8 b) { set(b, BC.lo); }  // SET b, C
void CPU::opCB_setD(u8 b) { set(b, DE.hi); }  // SET b, D
void CPU::opCB_setE(u8 b) { set(b, DE.lo); }  // SET b, E
void CPU::opCB_setH(u8 b) { set(b, HL.hi); }  // SET b, H
void CPU::opCB_setL(u8 b) { set(b, HL.lo); }  // SET b, L
void CPU::opCB_setHL(u8 b) {
    u8 val = read(HL.pair);
    set(b, val);
    write(HL.pair, val);
}  // SET b, (HL)
void CPU::opCB_setA(u8 b) { set(b, AF.hi); }  // SET b, A