#include "cpu.hpp"

#include <cstdio>

void CPU::init(Memory* memory, Debugger* debugger) {
    this->memory = memory;
    this->debugger = debugger;
    restart();

    opcodes[0x00] = &CPU::op00;
    opcodes[0x01] = &CPU::op01;
    opcodes[0x02] = &CPU::op02;
    opcodes[0x03] = &CPU::op03;
    opcodes[0x04] = &CPU::op04;
    opcodes[0x05] = &CPU::op05;
    opcodes[0x06] = &CPU::op06;
    opcodes[0x07] = &CPU::op07;
    opcodes[0x08] = &CPU::op08;
    opcodes[0x09] = &CPU::op09;
    opcodes[0x0A] = &CPU::op0A;
    opcodes[0x0B] = &CPU::op0B;
    opcodes[0x0C] = &CPU::op0C;
    opcodes[0x0D] = &CPU::op0D;
    opcodes[0x0E] = &CPU::op0E;
    opcodes[0x0F] = &CPU::op0F;

    opcodes[0x10] = &CPU::op10;
    opcodes[0x11] = &CPU::op11;
    opcodes[0x12] = &CPU::op12;
    opcodes[0x13] = &CPU::op13;
    opcodes[0x14] = &CPU::op14;
    opcodes[0x15] = &CPU::op15;
    opcodes[0x16] = &CPU::op16;
    opcodes[0x17] = &CPU::op17;
    opcodes[0x18] = &CPU::op18;
    opcodes[0x19] = &CPU::op19;
    opcodes[0x1A] = &CPU::op1A;
    opcodes[0x1B] = &CPU::op1B;
    opcodes[0x1C] = &CPU::op1C;
    opcodes[0x1D] = &CPU::op1D;
    opcodes[0x1E] = &CPU::op1E;
    opcodes[0x1F] = &CPU::op1F;

    opcodes[0x20] = &CPU::op20;
    opcodes[0x21] = &CPU::op21;
    opcodes[0x22] = &CPU::op22;
    opcodes[0x23] = &CPU::op23;
    opcodes[0x24] = &CPU::op24;
    opcodes[0x25] = &CPU::op25;
    opcodes[0x26] = &CPU::op26;
    opcodes[0x27] = &CPU::op27;
    opcodes[0x28] = &CPU::op28;
    opcodes[0x29] = &CPU::op29;
    opcodes[0x2A] = &CPU::op2A;
    opcodes[0x2B] = &CPU::op2B;
    opcodes[0x2C] = &CPU::op2C;
    opcodes[0x2D] = &CPU::op2D;
    opcodes[0x2E] = &CPU::op2E;
    opcodes[0x2F] = &CPU::op2F;

    opcodes[0x30] = &CPU::op30;
    opcodes[0x31] = &CPU::op31;
    opcodes[0x32] = &CPU::op32;
    opcodes[0x33] = &CPU::op33;
    opcodes[0x34] = &CPU::op34;
    opcodes[0x35] = &CPU::op35;
    opcodes[0x36] = &CPU::op36;
    opcodes[0x37] = &CPU::op37;
    opcodes[0x38] = &CPU::op38;
    opcodes[0x39] = &CPU::op39;
    opcodes[0x3A] = &CPU::op3A;
    opcodes[0x3B] = &CPU::op3B;
    opcodes[0x3C] = &CPU::op3C;
    opcodes[0x3D] = &CPU::op3D;
    opcodes[0x3E] = &CPU::op3E;
    opcodes[0x3F] = &CPU::op3F;

    opcodes[0x40] = &CPU::op40;
    opcodes[0x41] = &CPU::op41;
    opcodes[0x42] = &CPU::op42;
    opcodes[0x43] = &CPU::op43;
    opcodes[0x44] = &CPU::op44;
    opcodes[0x45] = &CPU::op45;
    opcodes[0x46] = &CPU::op46;
    opcodes[0x47] = &CPU::op47;
    opcodes[0x48] = &CPU::op48;
    opcodes[0x49] = &CPU::op49;
    opcodes[0x4A] = &CPU::op4A;
    opcodes[0x4B] = &CPU::op4B;
    opcodes[0x4C] = &CPU::op4C;
    opcodes[0x4D] = &CPU::op4D;
    opcodes[0x4E] = &CPU::op4E;
    opcodes[0x4F] = &CPU::op4F;

    opcodes[0x50] = &CPU::op50;
    opcodes[0x51] = &CPU::op51;
    opcodes[0x52] = &CPU::op52;
    opcodes[0x53] = &CPU::op53;
    opcodes[0x54] = &CPU::op54;
    opcodes[0x55] = &CPU::op55;
    opcodes[0x56] = &CPU::op56;
    opcodes[0x57] = &CPU::op57;
    opcodes[0x58] = &CPU::op58;
    opcodes[0x59] = &CPU::op59;
    opcodes[0x5A] = &CPU::op5A;
    opcodes[0x5B] = &CPU::op5B;
    opcodes[0x5C] = &CPU::op5C;
    opcodes[0x5D] = &CPU::op5D;
    opcodes[0x5E] = &CPU::op5E;
    opcodes[0x5F] = &CPU::op5F;

    opcodes[0x60] = &CPU::op60;
    opcodes[0x61] = &CPU::op61;
    opcodes[0x62] = &CPU::op62;
    opcodes[0x63] = &CPU::op63;
    opcodes[0x64] = &CPU::op64;
    opcodes[0x65] = &CPU::op65;
    opcodes[0x66] = &CPU::op66;
    opcodes[0x67] = &CPU::op67;
    opcodes[0x68] = &CPU::op68;
    opcodes[0x69] = &CPU::op69;
    opcodes[0x6A] = &CPU::op6A;
    opcodes[0x6B] = &CPU::op6B;
    opcodes[0x6C] = &CPU::op6C;
    opcodes[0x6D] = &CPU::op6D;
    opcodes[0x6E] = &CPU::op6E;
    opcodes[0x6F] = &CPU::op6F;

    opcodes[0x70] = &CPU::op70;
    opcodes[0x71] = &CPU::op71;
    opcodes[0x72] = &CPU::op72;
    opcodes[0x73] = &CPU::op73;
    opcodes[0x74] = &CPU::op74;
    opcodes[0x75] = &CPU::op75;
    opcodes[0x76] = &CPU::op76;
    opcodes[0x77] = &CPU::op77;
    opcodes[0x78] = &CPU::op78;
    opcodes[0x79] = &CPU::op79;
    opcodes[0x7A] = &CPU::op7A;
    opcodes[0x7B] = &CPU::op7B;
    opcodes[0x7C] = &CPU::op7C;
    opcodes[0x7D] = &CPU::op7D;
    opcodes[0x7E] = &CPU::op7E;
    opcodes[0x7F] = &CPU::op7F;

    opcodes[0x80] = &CPU::op80;
    opcodes[0x81] = &CPU::op81;
    opcodes[0x82] = &CPU::op82;
    opcodes[0x83] = &CPU::op83;
    opcodes[0x84] = &CPU::op84;
    opcodes[0x85] = &CPU::op85;
    opcodes[0x86] = &CPU::op86;
    opcodes[0x87] = &CPU::op87;
    opcodes[0x88] = &CPU::op88;
    opcodes[0x89] = &CPU::op89;
    opcodes[0x8A] = &CPU::op8A;
    opcodes[0x8B] = &CPU::op8B;
    opcodes[0x8C] = &CPU::op8C;
    opcodes[0x8D] = &CPU::op8D;
    opcodes[0x8E] = &CPU::op8E;
    opcodes[0x8F] = &CPU::op8F;

    opcodes[0x90] = &CPU::op90;
    opcodes[0x91] = &CPU::op91;
    opcodes[0x92] = &CPU::op92;
    opcodes[0x93] = &CPU::op93;
    opcodes[0x94] = &CPU::op94;
    opcodes[0x95] = &CPU::op95;
    opcodes[0x96] = &CPU::op96;
    opcodes[0x97] = &CPU::op97;
    opcodes[0x98] = &CPU::op98;
    opcodes[0x99] = &CPU::op99;
    opcodes[0x9A] = &CPU::op9A;
    opcodes[0x9B] = &CPU::op9B;
    opcodes[0x9C] = &CPU::op9C;
    opcodes[0x9D] = &CPU::op9D;
    opcodes[0x9E] = &CPU::op9E;
    opcodes[0x9F] = &CPU::op9F;

    opcodes[0xA0] = &CPU::opA0;
    opcodes[0xA1] = &CPU::opA1;
    opcodes[0xA2] = &CPU::opA2;
    opcodes[0xA3] = &CPU::opA3;
    opcodes[0xA4] = &CPU::opA4;
    opcodes[0xA5] = &CPU::opA5;
    opcodes[0xA6] = &CPU::opA6;
    opcodes[0xA7] = &CPU::opA7;
    opcodes[0xA8] = &CPU::opA8;
    opcodes[0xA9] = &CPU::opA9;
    opcodes[0xAA] = &CPU::opAA;
    opcodes[0xAB] = &CPU::opAB;
    opcodes[0xAC] = &CPU::opAC;
    opcodes[0xAD] = &CPU::opAD;
    opcodes[0xAE] = &CPU::opAE;
    opcodes[0xAF] = &CPU::opAF;

    opcodes[0xB0] = &CPU::opB0;
    opcodes[0xB1] = &CPU::opB1;
    opcodes[0xB2] = &CPU::opB2;
    opcodes[0xB3] = &CPU::opB3;
    opcodes[0xB4] = &CPU::opB4;
    opcodes[0xB5] = &CPU::opB5;
    opcodes[0xB6] = &CPU::opB6;
    opcodes[0xB7] = &CPU::opB7;
    opcodes[0xB8] = &CPU::opB8;
    opcodes[0xB9] = &CPU::opB9;
    opcodes[0xBA] = &CPU::opBA;
    opcodes[0xBB] = &CPU::opBB;
    opcodes[0xBC] = &CPU::opBC;
    opcodes[0xBD] = &CPU::opBD;
    opcodes[0xBE] = &CPU::opBE;
    opcodes[0xBF] = &CPU::opBF;

    opcodes[0xC0] = &CPU::opC0;
    opcodes[0xC1] = &CPU::opC1;
    opcodes[0xC2] = &CPU::opC2;
    opcodes[0xC3] = &CPU::opC3;
    opcodes[0xC4] = &CPU::opC4;
    opcodes[0xC5] = &CPU::opC5;
    opcodes[0xC6] = &CPU::opC6;
    opcodes[0xC7] = &CPU::opC7;
    opcodes[0xC8] = &CPU::opC8;
    opcodes[0xC9] = &CPU::opC9;
    opcodes[0xCA] = &CPU::opCA;
    opcodes[0xCB] = &CPU::opCB;
    opcodes[0xCC] = &CPU::opCC;
    opcodes[0xCD] = &CPU::opCD;
    opcodes[0xCE] = &CPU::opCE;
    opcodes[0xCF] = &CPU::opCF;

    opcodes[0xD0] = &CPU::opD0;
    opcodes[0xD1] = &CPU::opD1;
    opcodes[0xD2] = &CPU::opD2;
    opcodes[0xD3] = &CPU::opD3;
    opcodes[0xD4] = &CPU::opD4;
    opcodes[0xD5] = &CPU::opD5;
    opcodes[0xD6] = &CPU::opD6;
    opcodes[0xD7] = &CPU::opD7;
    opcodes[0xD8] = &CPU::opD8;
    opcodes[0xD9] = &CPU::opD9;
    opcodes[0xDA] = &CPU::opDA;
    opcodes[0xDB] = &CPU::opDB;
    opcodes[0xDC] = &CPU::opDC;
    opcodes[0xDD] = &CPU::opDD;
    opcodes[0xDE] = &CPU::opDE;
    opcodes[0xDF] = &CPU::opDF;

    opcodes[0xE0] = &CPU::opE0;
    opcodes[0xE1] = &CPU::opE1;
    opcodes[0xE2] = &CPU::opE2;
    opcodes[0xE3] = &CPU::opE3;
    opcodes[0xE4] = &CPU::opE4;
    opcodes[0xE5] = &CPU::opE5;
    opcodes[0xE6] = &CPU::opE6;
    opcodes[0xE7] = &CPU::opE7;
    opcodes[0xE8] = &CPU::opE8;
    opcodes[0xE9] = &CPU::opE9;
    opcodes[0xEA] = &CPU::opEA;
    opcodes[0xEB] = &CPU::opEB;
    opcodes[0xEC] = &CPU::opEC;
    opcodes[0xED] = &CPU::opED;
    opcodes[0xEE] = &CPU::opEE;
    opcodes[0xEF] = &CPU::opEF;

    opcodes[0xF0] = &CPU::opF0;
    opcodes[0xF1] = &CPU::opF1;
    opcodes[0xF2] = &CPU::opF2;
    opcodes[0xF3] = &CPU::opF3;
    opcodes[0xF4] = &CPU::opF4;
    opcodes[0xF5] = &CPU::opF5;
    opcodes[0xF6] = &CPU::opF6;
    opcodes[0xF7] = &CPU::opF7;
    opcodes[0xF8] = &CPU::opF8;
    opcodes[0xF9] = &CPU::opF9;
    opcodes[0xFA] = &CPU::opFA;
    opcodes[0xFB] = &CPU::opFB;
    opcodes[0xFC] = &CPU::opFC;
    opcodes[0xFD] = &CPU::opFD;
    opcodes[0xFE] = &CPU::opFE;
    opcodes[0xFF] = &CPU::opFF;

    // CB Opcodes
    opcodesCB[0x00] = &CPU::opCB00;
    opcodesCB[0x01] = &CPU::opCB01;
    opcodesCB[0x02] = &CPU::opCB02;
    opcodesCB[0x03] = &CPU::opCB03;
    opcodesCB[0x04] = &CPU::opCB04;
    opcodesCB[0x05] = &CPU::opCB05;
    opcodesCB[0x06] = &CPU::opCB06;
    opcodesCB[0x07] = &CPU::opCB07;
    opcodesCB[0x08] = &CPU::opCB08;
    opcodesCB[0x09] = &CPU::opCB09;
    opcodesCB[0x0A] = &CPU::opCB0A;
    opcodesCB[0x0B] = &CPU::opCB0B;
    opcodesCB[0x0C] = &CPU::opCB0C;
    opcodesCB[0x0D] = &CPU::opCB0D;
    opcodesCB[0x0E] = &CPU::opCB0E;
    opcodesCB[0x0F] = &CPU::opCB0F;

    opcodesCB[0x10] = &CPU::opCB10;
    opcodesCB[0x11] = &CPU::opCB11;
    opcodesCB[0x12] = &CPU::opCB12;
    opcodesCB[0x13] = &CPU::opCB13;
    opcodesCB[0x14] = &CPU::opCB14;
    opcodesCB[0x15] = &CPU::opCB15;
    opcodesCB[0x16] = &CPU::opCB16;
    opcodesCB[0x17] = &CPU::opCB17;
    opcodesCB[0x18] = &CPU::opCB18;
    opcodesCB[0x19] = &CPU::opCB19;
    opcodesCB[0x1A] = &CPU::opCB1A;
    opcodesCB[0x1B] = &CPU::opCB1B;
    opcodesCB[0x1C] = &CPU::opCB1C;
    opcodesCB[0x1D] = &CPU::opCB1D;
    opcodesCB[0x1E] = &CPU::opCB1E;
    opcodesCB[0x1F] = &CPU::opCB1F;

    opcodesCB[0x20] = &CPU::opCB20;
    opcodesCB[0x21] = &CPU::opCB21;
    opcodesCB[0x22] = &CPU::opCB22;
    opcodesCB[0x23] = &CPU::opCB23;
    opcodesCB[0x24] = &CPU::opCB24;
    opcodesCB[0x25] = &CPU::opCB25;
    opcodesCB[0x26] = &CPU::opCB26;
    opcodesCB[0x27] = &CPU::opCB27;
    opcodesCB[0x28] = &CPU::opCB28;
    opcodesCB[0x29] = &CPU::opCB29;
    opcodesCB[0x2A] = &CPU::opCB2A;
    opcodesCB[0x2B] = &CPU::opCB2B;
    opcodesCB[0x2C] = &CPU::opCB2C;
    opcodesCB[0x2D] = &CPU::opCB2D;
    opcodesCB[0x2E] = &CPU::opCB2E;
    opcodesCB[0x2F] = &CPU::opCB2F;

    opcodesCB[0x30] = &CPU::opCB30;
    opcodesCB[0x31] = &CPU::opCB31;
    opcodesCB[0x32] = &CPU::opCB32;
    opcodesCB[0x33] = &CPU::opCB33;
    opcodesCB[0x34] = &CPU::opCB34;
    opcodesCB[0x35] = &CPU::opCB35;
    opcodesCB[0x36] = &CPU::opCB36;
    opcodesCB[0x37] = &CPU::opCB37;
    opcodesCB[0x38] = &CPU::opCB38;
    opcodesCB[0x39] = &CPU::opCB39;
    opcodesCB[0x3A] = &CPU::opCB3A;
    opcodesCB[0x3B] = &CPU::opCB3B;
    opcodesCB[0x3C] = &CPU::opCB3C;
    opcodesCB[0x3D] = &CPU::opCB3D;
    opcodesCB[0x3E] = &CPU::opCB3E;
    opcodesCB[0x3F] = &CPU::opCB3F;

    bitOpcodesCB[0x00] = opCB_bitB;
    bitOpcodesCB[0x01] = opCB_bitC;
    bitOpcodesCB[0x02] = opCB_bitD;
    bitOpcodesCB[0x03] = opCB_bitE;
    bitOpcodesCB[0x04] = opCB_bitH;
    bitOpcodesCB[0x05] = opCB_bitL;
    bitOpcodesCB[0x06] = opCB_bitHL;
    bitOpcodesCB[0x07] = opCB_bitA;

    bitOpcodesCB[0x08] = opCB_resB;
    bitOpcodesCB[0x09] = opCB_resC;
    bitOpcodesCB[0x0A] = opCB_resD;
    bitOpcodesCB[0x0B] = opCB_resE;
    bitOpcodesCB[0x0C] = opCB_resH;
    bitOpcodesCB[0x0D] = opCB_resL;
    bitOpcodesCB[0x0E] = opCB_resHL;
    bitOpcodesCB[0x0F] = opCB_resA;

    bitOpcodesCB[0x10] = opCB_setB;
    bitOpcodesCB[0x11] = opCB_setC;
    bitOpcodesCB[0x12] = opCB_setD;
    bitOpcodesCB[0x13] = opCB_setE;
    bitOpcodesCB[0x14] = opCB_setH;
    bitOpcodesCB[0x15] = opCB_setL;
    bitOpcodesCB[0x16] = opCB_setHL;
    bitOpcodesCB[0x17] = opCB_setA;
}

void CPU::restart() {
    AF.pair = 0x01B0;
    BC.pair = 0x0013;
    DE.pair = 0x00D8;
    HL.pair = 0x014D;
    SP = 0xFFFE;
    PC = 0x0100;
    ime = false;
    imeScheduled = false;
    halted = false;
    haltBug = false;

    cycleCnt = 0;

    readCycle = 0;
}

bool test = false;
extern u8 cnt;
void CPU::handle_interrupts() {
    // TODO handle weird push to IE case
    u8 ifReg = memory->read(IOReg::IF_REG);
    u8 interrupts = ifReg & memory->read(IOReg::IE_REG) & 0x1F;
    if (interrupts) {
        // TODO only add extra cycle if emulating CGB
        // if (halted) cycleAcc++;  // Extra cycle if cpu is in halt mode

        if (ime) {
            for (int i = 0; i < 5; i++) {
                if (interrupts & (1 << i)) {
                    printf("Interrupt! %02x from %02x\n", i * 0x8 + 0x40, PC);
                    call(i * 0x8 + 0x40);
                    ime = false;
                    write(IOReg::IF_REG, ifReg & ~(1 << i));

                    cycleCnt++;  // Interrupt handling takes total of 5 cycles (+1 if halted)
                    break;
                }
            }
        }
        halted = false;
    }
    if (imeScheduled) {
        ime = true;
        imeScheduled = false;
    }
}

bool CPU::isFetching() { return cycleCnt == 0; }

void CPU::emulate_cycle() {
    if (isFetching()) {
        if (test) printf("cc=%d PC=%02x\n", cnt + 2, PC);

        if (halted) {
            return;
        }

        u8 op = n();
        if (debugger->is_paused()) {
            printf("fetch--%04x-%02x\n", PC - 1, op);
        }

        if (haltBug) {
            PC--;
            haltBug = false;
        }
        (this->*opcodes[op])();
    }

    cycleCnt--;
    if (cycleCnt == 0) {
        if (debugger->is_paused()) {
            printf("---------------\n");
            debugger->print_instr();
        }
        debugger->update_instr(PC);

        // if (PC == 0x210 && AF.hi == 0x98 && cnt == 8) {
        // if (memory->read(IOReg::LY_REG) == 0x8F && cnt == 8) {
        // if (PC == 0x212 && AF.hi == 0x40) {
        // if (PC == 0x21C && AF.hi == 0) {
        // if (PC == 0x22d) {
        // if (PC == 0x1D2 && memory->read(IOReg::LY_REG) == 0x00) {
        // if (PC == 0x196) {
        // if (PC == 0x1C1) {
        // debugger->pause_exec();
        // test = true;
        // }
    }

    if (readCycle > 0) {
        readCycle--;
        if (readCycle == 0) {
            readCallback(this);
        }
    }
}

void CPU::set_flag(u8 flag, bool set) { AF.lo = (AF.lo & ~flag) | (flag * set); }
bool CPU::check_flag(u8 flag) { return (AF.lo & flag) != 0; }
u8 CPU::n() {
    debugger->inc_instr_bytes();
    return read(PC++);
}
u16 CPU::nn() {
    u16 res = read(PC);
    PC++;
    res |= read(PC) << 8;
    PC++;
    debugger->inc_instr_bytes();
    debugger->inc_instr_bytes();
    return res;
}
u8 CPU::read(u16 addr) {
    cycleCnt++;
    return memory->read(addr);
}
void CPU::write(u16 addr, u8 val) {
    cycleCnt++;
    memory->write(addr, val);
}
void CPU::skd_read(u8& dest, u16 addr, ReadCallback&& callback) {
    skd_read(dest, addr);
    readCycle = cycleCnt;
    readCallback = callback;
}
void CPU::skd_read(u8& dest, u16 addr) {
    memory->schedule_read(&dest, addr, cycleCnt);
    cycleCnt++;
}
void CPU::skd_write(u16 addr, u8& val) {
    memory->schedule_write(addr, &val, cycleCnt);
    cycleCnt++;
}

// ----- Intructions -----
void CPU::op00() {}                       // NOP
void CPU::op10() { printf("stop!!\n"); }  // STOP
void CPU::op76() {
    u8 interrupts = memory->read(IOReg::IF_REG) & memory->read(IOReg::IE_REG) & 0x1F;
    halted = ime || !interrupts;
    if (!halted) haltBug = true;
}  // HALT

// ----- Interrupts -----
void CPU::opF3() { ime = false; }          // DI
void CPU::opFB() { imeScheduled = true; }  // EI

// ----- 8-bit -----

// LD (16), A
void CPU::op02() { write(BC.pair, AF.hi); }    // LD (BC), A
void CPU::op12() { write(DE.pair, AF.hi); }    // LD (DE), A
void CPU::op22() { write(HL.pair++, AF.hi); }  // LD (HL+), A
void CPU::op32() { write(HL.pair--, AF.hi); }  // LD (HL-), A

// 8-bit load
void CPU::op06() { BC.hi = n(); }  // LD B, n
void CPU::op16() { DE.hi = n(); }  // LD D, n
void CPU::op26() { HL.hi = n(); }  // LD H, n
void CPU::op36() {
    valReg = n();
    skd_write(HL.pair, valReg);
}  // LD (HL), n
void CPU::op0E() { BC.lo = n(); }  // LD C, n
void CPU::op1E() { DE.lo = n(); }  // LD E, n
void CPU::op2E() { HL.lo = n(); }  // LD L, n
void CPU::op3E() { AF.hi = n(); }  // LD A, n

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
void CPU::opE0() { skd_write(0xFF00 + n(), AF.hi); }  // LDH ($FF00 + n), A
void CPU::opF0() { skd_read(AF.hi, 0xFF00 + n()); }   // LDH A, ($FF00 + n)

// load $FF00 + C
void CPU::opE2() { write(BC.lo + 0xFF00, AF.hi); }  // LD (C), A [A -> $FF00 + C]
void CPU::opF2() { AF.hi = read(BC.lo + 0xFF00); }  // LD A, (C) [$FF00 + C -> A]

void CPU::opEA() { skd_write(nn(), AF.hi); }  // LD (nn), A

void CPU::opFA() { skd_read(AF.hi, nn()); }

// ----- 16-bit -----

// LD n, nn
void CPU::op01() { BC.pair = nn(); }  // LD BC, nn
void CPU::op11() { DE.pair = nn(); }  // LD DE, nn
void CPU::op21() { HL.pair = nn(); }  // LD HL, nn
void CPU::op31() { SP = nn(); }       // LD SP, nn

// pop
u16 CPU::pop() {
    u16 res = read(SP);
    SP++;
    res |= read(SP) << 8;
    SP++;
    return res;
}
void CPU::opC1() { BC.pair = pop(); }  // POP BC
void CPU::opD1() { DE.pair = pop(); }  // POP DE
void CPU::opE1() { HL.pair = pop(); }  // POP HL
void CPU::opF1() {
    AF.lo = (read(SP++) & 0xF0) | (AF.lo & 0xF);
    AF.hi = read(SP++);
}  // POP AF

// push
void CPU::push(u16 reg) {
    cycleCnt++;  // Stack push takes an extra cycle
    write(--SP, reg >> 8);
    write(--SP, reg & 0xFF);
}
void CPU::opC5() { push(BC.pair); }  // PUSH BC
void CPU::opD5() { push(DE.pair); }  // PUSH DE
void CPU::opE5() { push(HL.pair); }  // PUSH HL
void CPU::opF5() { push(AF.pair); }  // PUSH AF

void CPU::op08() {
    u16 addr = nn();
    write(addr, SP & 0xFF);
    write(addr + 1, SP >> 8);
}  // LD (nn), SP

void CPU::opF9() {
    cycleCnt++;
    SP = HL.pair;
}  // LD SP, HL

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
    set_flag(C_FLAG, val + carry > 0xFF - AF.hi);
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
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (AF.hi & 0xF) < ((val & 0xF) + carry));
    set_flag(C_FLAG, AF.hi < val + carry);
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
    set_flag(H_FLAG, !(reg & 0xF));
}
void CPU::op04() { inc8(BC.hi); }  // INC B
void CPU::op0C() { inc8(BC.lo); }  // INC C
void CPU::op14() { inc8(DE.hi); }  // INC D
void CPU::op1C() { inc8(DE.lo); }  // INC E
void CPU::op24() { inc8(HL.hi); }  // INC H
void CPU::op2C() { inc8(HL.lo); }  // INC L
void CPU::op34() {
    valReg = read(HL.pair);
    inc8(valReg);
    skd_write(HL.pair, valReg);
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
    valReg = read(HL.pair);
    dec8(valReg);
    skd_write(HL.pair, valReg);
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
    cycleCnt++;
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
    // stackPtr + off & 0xF
    set_flag(H_FLAG, (off & 0xF) > 0xF - (stackPtr & 0xF));
    set_flag(C_FLAG, (off & 0xFF) > 0xFF - (stackPtr & 0xFF));
    // set_flag(H_FLAG, ((stackPtr ^ off) & 0x10) == 0x10);
    // set_flag(C_FLAG, ((stackPtr ^ off) & 0x100) == 0x100);

    cycleCnt++;
    return res;
}
void CPU::opE8() {
    cycleCnt++;
    SP = addSP_n();
}  // ADD SP, n
void CPU::opF8() { HL.pair = addSP_n(); }  // LD HL, SP + n

// INC nn
void CPU::op03() {
    cycleCnt++;
    BC.pair++;
}  // INC BC
void CPU::op13() {
    cycleCnt++;
    DE.pair++;
}  // INC DE
void CPU::op23() {
    cycleCnt++;
    HL.pair++;
}  // INC HL
void CPU::op33() {
    cycleCnt++;
    SP++;
}  // INC SP

void CPU::op0B() {
    cycleCnt++;
    BC.pair--;
}  // DEC BC
void CPU::op1B() {
    cycleCnt++;
    DE.pair--;
}  // DEC DE
void CPU::op2B() {
    cycleCnt++;
    HL.pair--;
}  // DEC HL
void CPU::op3B() {
    cycleCnt++;
    SP--;
}  // DEC SP

// ----- Misc ALU -----

// Rotate Left
void CPU::op07() {
    rlc8(AF.hi);
    set_flag(Z_FLAG, false);
}  // RLCA
void CPU::op17() {
    rl8(AF.hi);
    set_flag(Z_FLAG, false);
}  // RLA

// Rotate right
void CPU::op0F() {
    rrc8(AF.hi);
    set_flag(Z_FLAG, false);

}  // RRCA
void CPU::op1F() {
    rr8(AF.hi);
    set_flag(Z_FLAG, false);
}  // RRA

void CPU::op27() {
    if (check_flag(N_FLAG)) {  // Subtraction
        if (check_flag(C_FLAG)) AF.hi -= 0x6 << 4;
        if (check_flag(H_FLAG)) AF.hi -= 0x6;
    } else {  // Addition
        if (check_flag(C_FLAG) || AF.hi > 0x99) {
            AF.hi += 0x6 << 4;
            set_flag(C_FLAG, true);
        }
        if (check_flag(H_FLAG) || (AF.hi & 0xF) > 0x9) AF.hi += 0x6;
    }
    set_flag(Z_FLAG, AF.hi == 0);
    set_flag(H_FLAG, false);
}  // DAA
void CPU::op37() {
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, true);
}  // SCF

void CPU::op2F() {
    AF.hi = ~AF.hi;
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, true);
}  // CPL
void CPU::op3F() {
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, !check_flag(C_FLAG));
}  // CCF

// ----- Jump -----
void CPU::jump(u16 addr) {
    cycleCnt++;
    PC = addr;
}
void CPU::opC3() { jump(nn()); }  // JP nn

void CPU::opC2() {
    u16 addr = nn();
    if (!check_flag(Z_FLAG)) {
        jump(addr);
    }
}  // JP NZ, nn
void CPU::opD2() {
    u16 addr = nn();
    if (!check_flag(C_FLAG)) {
        jump(addr);
    }
}  // JP NC, nn
void CPU::opCA() {
    u16 addr = nn();
    if (check_flag(Z_FLAG)) {
        jump(addr);
    }
}  // JP Z, nn
void CPU::opDA() {
    u16 addr = nn();
    if (check_flag(C_FLAG)) {
        jump(addr);
    }
}  // JP C, nn

void CPU::opE9() { PC = HL.pair; }  // JP HL

void CPU::op18() {
    s8 val = n();
    jump(PC + val);
}  // JR n
void CPU::op20() {
    s8 val = n();
    if (!check_flag(Z_FLAG)) {
        jump(PC + val);
    }
}  // JR NZ, n
void CPU::op30() {
    s8 val = n();
    if (!check_flag(C_FLAG)) {
        jump(PC + val);
    }
}  // JR NC, n
void CPU::op28() {
    s8 val = n();
    if (check_flag(Z_FLAG)) {
        jump(PC + val);
    }
}  // JR Z, n
void CPU::op38() {
    s8 val = n();
    if (check_flag(C_FLAG)) {
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
        call(addr);
    }
}  // CALL NZ, nn
void CPU::opD4() {
    u16 addr = nn();
    if (!check_flag(C_FLAG)) {
        call(addr);
    }

}  // CALL NC, nn
void CPU::opCC() {
    u16 addr = nn();
    if (check_flag(Z_FLAG)) {
        call(addr);
    }
}  // CALL Z, nn
void CPU::opDC() {
    u16 addr = nn();
    if (check_flag(C_FLAG)) {
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
    cycleCnt++;
    if (!check_flag(Z_FLAG)) {
        jump(pop());
    }
}  // RET NZ
void CPU::opD0() {
    cycleCnt++;
    if (!check_flag(C_FLAG)) {
        jump(pop());
    }
}  // RET NC
void CPU::opC8() {
    cycleCnt++;
    if (check_flag(Z_FLAG)) {
        jump(pop());
    }
}  // RET Z
void CPU::opD8() {
    cycleCnt++;
    if (check_flag(C_FLAG)) {
        jump(pop());
    }
}  // RET C

void CPU::opD9() {
    jump(pop());
    ime = true;
}  // RETI

// ----- Error Opcodes -----
void CPU::freeze() {
    printf("Invalid opcode, freezing...\n");
    debugger->pause_exec();
}
void CPU::opD3() { freeze(); }
void CPU::opE3() { freeze(); }
void CPU::opE4() { freeze(); }
void CPU::opF4() { freeze(); }
void CPU::opDB() { freeze(); }
void CPU::opEB() { freeze(); }
void CPU::opEC() { freeze(); }
void CPU::opFC() { freeze(); }
void CPU::opDD() { freeze(); }
void CPU::opED() { freeze(); }
void CPU::opFD() { freeze(); }

// ----- CB-prefix -----
void CPU::opCB() {
    u8 cbOp = n();
    if (cbOp < 0x40) {
        (this->*opcodesCB[cbOp])();
    } else {
        u8 index = ((cbOp / 0x40) - 1) * 0x8 + (cbOp % 0x8);
        u8 b = (cbOp / 0x8) % 0x8;
        (this->*bitOpcodesCB[index])(b);
    }
}  // CB-prefix

// SWAP n
void CPU::swap(u8& reg) {
    reg = ((reg & 0xF) << 4) | (reg >> 4);
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, 0);
    set_flag(H_FLAG, 0);
    set_flag(C_FLAG, 0);
}
void CPU::opCB30() { swap(BC.hi); }  // SWAP B
void CPU::opCB31() { swap(BC.lo); }  // SWAP C
void CPU::opCB32() { swap(DE.hi); }  // SWAP D
void CPU::opCB33() { swap(DE.lo); }  // SWAP E
void CPU::opCB34() { swap(HL.hi); }  // SWAP H
void CPU::opCB35() { swap(HL.lo); }  // SWAP L
void CPU::opCB36() {
    skd_read(valReg, HL.pair, [](CPU* c) { c->swap(c->valReg); });
    skd_write(HL.pair, valReg);
}  // SWAP (HL)
void CPU::opCB37() { swap(AF.hi); }  // SWAP A

//  RLC n
void CPU::rlc8(u8& reg) {
    set_flag(C_FLAG, reg >> 7);
    reg = (reg << 1) | (reg >> 7);
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}
void CPU::opCB00() { rlc8(BC.hi); }  // RLC B
void CPU::opCB01() { rlc8(BC.lo); }  // RLC C
void CPU::opCB02() { rlc8(DE.hi); }  // RLC D
void CPU::opCB03() { rlc8(DE.lo); }  // RLC E
void CPU::opCB04() { rlc8(HL.hi); }  // RLC H
void CPU::opCB05() { rlc8(HL.lo); }  // RLC L
void CPU::opCB06() {
    skd_read(valReg, HL.pair, [](CPU* c) { c->rlc8(c->valReg); });
    skd_write(HL.pair, valReg);
}  // RLC (HL)
void CPU::opCB07() { rlc8(AF.hi); }  // RLC A

//  RRC n
void CPU::rrc8(u8& reg) {
    set_flag(C_FLAG, reg & 0x1);
    reg = (reg << 7) | (reg >> 1);
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}
void CPU::opCB08() { rrc8(BC.hi); }  // RRC B
void CPU::opCB09() { rrc8(BC.lo); }  // RRC C
void CPU::opCB0A() { rrc8(DE.hi); }  // RRC D
void CPU::opCB0B() { rrc8(DE.lo); }  // RRC E
void CPU::opCB0C() { rrc8(HL.hi); }  // RRC H
void CPU::opCB0D() { rrc8(HL.lo); }  // RRC L
void CPU::opCB0E() {
    skd_read(valReg, HL.pair, [](CPU* c) { c->rrc8(c->valReg); });
    skd_write(HL.pair, valReg);
}  // RRC (HL)
void CPU::opCB0F() { rrc8(AF.hi); }  // RRC A

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
    skd_read(valReg, HL.pair, [](CPU* c) { c->rl8(c->valReg); });
    skd_write(HL.pair, valReg);
}  // RL (HL)
void CPU::opCB17() { rl8(AF.hi); }  // RL A

// RR n
void CPU::rr8(u8& reg) {
    u8 res = (check_flag(C_FLAG) << 7) | (reg >> 1);
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, reg & 0x1);
    reg = res;
}
void CPU::opCB18() { rr8(BC.hi); }  // RR B
void CPU::opCB19() { rr8(BC.lo); }  // RR C
void CPU::opCB1A() { rr8(DE.hi); }  // RR D
void CPU::opCB1B() { rr8(DE.lo); }  // RR E
void CPU::opCB1C() { rr8(HL.hi); }  // RR H
void CPU::opCB1D() { rr8(HL.lo); }  // RR L
void CPU::opCB1E() {
    skd_read(valReg, HL.pair, [](CPU* c) { c->rr8(c->valReg); });
    skd_write(HL.pair, valReg);
}  // RR (HL)
void CPU::opCB1F() { rr8(AF.hi); }  // RR A

// SLA n
void CPU::sla8(u8& reg) {
    set_flag(C_FLAG, reg >> 7);
    reg <<= 1;
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}
void CPU::opCB20() { sla8(BC.hi); }  // SLA B
void CPU::opCB21() { sla8(BC.lo); }  // SLA C
void CPU::opCB22() { sla8(DE.hi); }  // SLA D
void CPU::opCB23() { sla8(DE.lo); }  // SLA E
void CPU::opCB24() { sla8(HL.hi); }  // SLA H
void CPU::opCB25() { sla8(HL.lo); }  // SLA L
void CPU::opCB26() {
    skd_read(valReg, HL.pair, [](CPU* c) { c->sla8(c->valReg); });
    skd_write(HL.pair, valReg);
}  // SLA (HL)
void CPU::opCB27() { sla8(AF.hi); }  // SLA A

// SRA n
void CPU::sra8(u8& reg) {
    set_flag(C_FLAG, reg & 0x1);
    reg = (s8)reg >> 1;  // Arithmetic right shift
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}
void CPU::opCB28() { sra8(BC.hi); }  // SRA B
void CPU::opCB29() { sra8(BC.lo); }  // SRA C
void CPU::opCB2A() { sra8(DE.hi); }  // SRA D
void CPU::opCB2B() { sra8(DE.lo); }  // SRA E
void CPU::opCB2C() { sra8(HL.hi); }  // SRA H
void CPU::opCB2D() { sra8(HL.lo); }  // SRA L
void CPU::opCB2E() {
    skd_read(valReg, HL.pair, [](CPU* c) { c->sra8(c->valReg); });
    skd_write(HL.pair, valReg);
}  // SRA (HL)
void CPU::opCB2F() { sra8(AF.hi); }  // SRA A

// SRL n
void CPU::srl8(u8& reg) {
    set_flag(C_FLAG, reg & 0x1);
    reg >>= 1;
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}
void CPU::opCB38() { srl8(BC.hi); }  // SRL B
void CPU::opCB39() { srl8(BC.lo); }  // SRL C
void CPU::opCB3A() { srl8(DE.hi); }  // SRL D
void CPU::opCB3B() { srl8(DE.lo); }  // SRL E
void CPU::opCB3C() { srl8(HL.hi); }  // SRL H
void CPU::opCB3D() { srl8(HL.lo); }  // SRL L
void CPU::opCB3E() {
    skd_read(valReg, HL.pair, [](CPU* c) { c->srl8(c->valReg); });
    skd_write(HL.pair, valReg);
}  // SRL (HL)
void CPU::opCB3F() { srl8(AF.hi); }  // SRL A

// BIT b, r
void CPU::bit(u8 b, u8 reg) {
    set_flag(Z_FLAG, (reg & (1 << b)) == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, true);
}
void CPU::opCB_bitB(u8 b) { bit(b, BC.hi); }  // BIT b, B
void CPU::opCB_bitC(u8 b) { bit(b, BC.lo); }  // BIT b, C
void CPU::opCB_bitD(u8 b) { bit(b, DE.hi); }  // BIT b, D
void CPU::opCB_bitE(u8 b) { bit(b, DE.lo); }  // BIT b, E
void CPU::opCB_bitH(u8 b) { bit(b, HL.hi); }  // BIT b, H
void CPU::opCB_bitL(u8 b) { bit(b, HL.lo); }  // BIT b, L
void CPU::opCB_bitHL(u8 b) {
    static u8 bitReg;
    bitReg = b;
    skd_read(valReg, HL.pair, [](CPU* c) { c->bit(bitReg, c->valReg); });
}  // BIT b, (HL)
void CPU::opCB_bitA(u8 b) { bit(b, AF.hi); }  // BIT b, A

// RES b, r
void CPU::res(u8 b, u8& reg) { reg &= ~(1 << b); }
void CPU::opCB_resB(u8 b) { res(b, BC.hi); }  // RES b, B
void CPU::opCB_resC(u8 b) { res(b, BC.lo); }  // RES b, C
void CPU::opCB_resD(u8 b) { res(b, DE.hi); }  // RES b, D
void CPU::opCB_resE(u8 b) { res(b, DE.lo); }  // RES b, E
void CPU::opCB_resH(u8 b) { res(b, HL.hi); }  // RES b, H
void CPU::opCB_resL(u8 b) { res(b, HL.lo); }  // RES b, L
void CPU::opCB_resHL(u8 b) {
    static u8 bitReg;
    bitReg = b;
    skd_read(valReg, HL.pair, [](CPU* c) { c->res(bitReg, c->valReg); });
    skd_write(HL.pair, valReg);
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
    static u8 bitReg;
    bitReg = b;
    skd_read(valReg, HL.pair, [](CPU* c) { c->set(bitReg, c->valReg); });
    skd_write(HL.pair, valReg);
}  // SET b, (HL)
void CPU::opCB_setA(u8 b) { set(b, AF.hi); }  // SET b, A