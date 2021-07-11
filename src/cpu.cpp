#include "cpu.hpp"

CPU::CPU(Memory& memory) : memory(&memory) {}

void CPU::restart() {
    if (memory->is_CGB()) {
        regs.AF = 0x1180;
        regs.BC = 0x0000;
        regs.DE = 0x0008;
        regs.HL = 0x007C;
    } else {
        regs.AF = 0x01B0;
        regs.BC = 0x0013;
        regs.DE = 0x00D8;
        regs.HL = 0x014D;
    }
    SP = 0xFFFE;
    PC = 0x0100;
    ime = false;
    imeScheduled = false;
    halted = false;
    haltBug = false;

    cycleCnt = 0;

    callbackCycle = 0;
}

void CPU::handle_interrupts() {
    u8 ifReg = memory->read(IOReg::IF_REG);
    u8 interrupts = ifReg & memory->read(IOReg::IE_REG) & 0x1F;
    if (interrupts) {
        // TODO only add extra cycle if emulating CGB
        // if (halted) cycleCnt++;  // Extra cycle if cpu is in halt mode

        if (ime) {
            for (int i = 0; i < 5; i++) {
                if (interrupts & (1 << i)) {
                    ime = false;

                    cycleCnt++;

                    // TODO test if PC push is cut halfway (msb is written but lsb is not)
                    static u8 msb;
                    msb = PC >> 8;
                    skd_write(--SP, msb);

                    if (SP == 0xFFFF) {
                        u8 pushIE = PC >> 8;
                        if ((ifReg & pushIE & 0x1F & (1 << i)) == 0) {
                            PC = 0x0000;
#if PLAYABLE
                            printf("Info: ie push cancel ocurred\n");
#endif
                            continue;
                        }
                    }
                    static u8 lsb;
                    lsb = PC & 0xFF;
                    skd_write(--SP, lsb);
                    PC = i * 0x8 + 0x40;

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

bool CPU::is_fetching() { return cycleCnt == 0; }

void CPU::emulate_cycle() {
    if (is_fetching()) {
        if (debugger->is_paused()) {
            debugger->print_info();
        }

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
        execute(op);
    }

    if (--cycleCnt == 0) {
        // PAUSE_EXEC_OCC(this, debugger, 0x40, 3300);
        // PAUSE_EXEC(this, debugger, 0x47F9);
        // if (memory->read(IOReg::LYC_REG) == 0x01) {
        // PAUSE_EXEC(this, debugger, 0x4950);
        // }
    }

    if (callbackCycle > 0) {
        callbackCycle--;
        if (callbackCycle == 0) {
            callback(this);
        }
    }
}

// clang-format off
void CPU::execute(u8 opcode) {
    switch (opcode) {
        case 0x00: break; // NOP
        case 0x10: // STOP
            // TODO implement stop functionality correctly
            // Other emulators seem to wait some amount of cycles before waking up (0x20000 on gambatte)
            // This allows double speed mode on CGB to work

            PC++;  // Stop instruction will skip the immediate next byte
            break;
        case 0x76:  // HALT
            halted = ime || !(memory->read(IOReg::IF_REG) & memory->read(IOReg::IE_REG) & 0x1F);
            if (!halted) haltBug = true;
            break;
        case 0xF3: ime = false; break;         // DI
        case 0xFB: imeScheduled = true; break; // EI
        // LD (reg16), A
        case 0x02: write(regs.BC, regs.A); break;   // LD (BC), A
        case 0x12: write(regs.DE, regs.A); break;   // LD (DE), A
        case 0x22: write(regs.HL++, regs.A); break; // LD (HL+), A
        case 0x32: write(regs.HL--, regs.A); break; // LD (HL-), A
        // LD reg8, n
        case 0x06: regs.B = n(); break; // LD B, n
        case 0x16: regs.D = n(); break; // LD D, n
        case 0x26: regs.H = n(); break; // LD H, n
        case 0x36:
            temp8 = n();
            skd_write(regs.HL, temp8);
            break; // LD (HL), n
        case 0x0E: regs.C = n(); break; // LD C, n
        case 0x1E: regs.E = n(); break; // LD E, n
        case 0x2E: regs.L = n(); break; // LD L, n
        case 0x3E: regs.A = n(); break; // LD A, n
        // LD A, (reg16)
        case 0x0A: regs.A = read(regs.BC); break;   // LD A, (BC)
        case 0x1A: regs.A = read(regs.DE); break;   // LD A, (DE)
        case 0x2A: regs.A = read(regs.HL++); break; // LD A, (HL+)
        case 0x3A: regs.A = read(regs.HL--); break; // LD A, (HL-)
        // LD B, r2
        case 0x40:
            #if !PLAYABLE
            debugger->pause_exec();
            #endif
            break; // LD B, B
        case 0x41: regs.B = regs.C; break;         // LD B, C
        case 0x42: regs.B = regs.D; break;         // LD B, D
        case 0x43: regs.B = regs.E; break;         // LD B, E
        case 0x44: regs.B = regs.H; break;         // LD B, H
        case 0x45: regs.B = regs.L; break;         // LD B, L
        case 0x46: regs.B = read(regs.HL); break;  // LD B, (HL)
        case 0x47: regs.B = regs.A; break;         // LD B, A
        // LD C, r2
        case 0x48: regs.C = regs.B; break;         // LD C, B
        case 0x49: break;                          // LD C, C
        case 0x4A: regs.C = regs.D; break;         // LD C, D
        case 0x4B: regs.C = regs.E; break;         // LD C, E
        case 0x4C: regs.C = regs.H; break;         // LD C, H
        case 0x4D: regs.C = regs.L; break;         // LD C, L
        case 0x4E: regs.C = read(regs.HL); break;  // LD C, (HL)
        case 0x4F: regs.C = regs.A; break;         // LD C, A
        // LD D, r2
        case 0x50: regs.D = regs.B; break;         // LD D, B
        case 0x51: regs.D = regs.C; break;         // LD D, C
        case 0x52: break;                          // LD D, D
        case 0x53: regs.D = regs.E; break;         // LD D, E
        case 0x54: regs.D = regs.H; break;         // LD D, H
        case 0x55: regs.D = regs.L; break;         // LD D, L
        case 0x56: regs.D = read(regs.HL); break;  // LD D, (HL)
        case 0x57: regs.D = regs.A; break;         // LD D, A
        // LD E, r2
        case 0x58: regs.E = regs.B; break;         // LD E, B
        case 0x59: regs.E = regs.C; break;         // LD E, C
        case 0x5A: regs.E = regs.D; break;         // LD E, D
        case 0x5B: break;                          // LD E, E
        case 0x5C: regs.E = regs.H; break;         // LD E, H
        case 0x5D: regs.E = regs.L; break;         // LD E, L
        case 0x5E: regs.E = read(regs.HL); break;  // LD E, (HL)
        case 0x5F: regs.E = regs.A; break;         // LD E, A
        // LD H, r2
        case 0x60: regs.H = regs.B; break;         // LD H, B
        case 0x61: regs.H = regs.C; break;         // LD H, C
        case 0x62: regs.H = regs.D; break;         // LD H, D
        case 0x63: regs.H = regs.E; break;         // LD H, E
        case 0x64: break;                          // LD H, H
        case 0x65: regs.H = regs.L; break;         // LD H, L
        case 0x66: regs.H = read(regs.HL); break;  // LD H, (HL)
        case 0x67: regs.H = regs.A; break;         // LD H, A
        // LD L, r2
        case 0x68: regs.L = regs.B; break;         // LD L, B
        case 0x69: regs.L = regs.C; break;         // LD L, C
        case 0x6A: regs.L = regs.D; break;         // LD L, D
        case 0x6B: regs.L = regs.E; break;         // LD L, E
        case 0x6C: regs.L = regs.H; break;         // LD L, H
        case 0x6D: break;                          // LD L, L
        case 0x6E: regs.L = read(regs.HL); break;  // LD L, (HL)
        case 0x6F: regs.L = regs.A; break;         // LD L, A
        // LD (HL), r2
        case 0x70: write(regs.HL, regs.B); break; // LD (HL), B
        case 0x71: write(regs.HL, regs.C); break; // LD (HL), C
        case 0x72: write(regs.HL, regs.D); break; // LD (HL), D
        case 0x73: write(regs.HL, regs.E); break; // LD (HL), E
        case 0x74: write(regs.HL, regs.H); break; // LD (HL), H
        case 0x75: write(regs.HL, regs.L); break; // LD (HL), L
        case 0x77: write(regs.HL, regs.A); break; // LD (HL), A
        // LD A, r2
        case 0x78: regs.A = regs.B; break;         // LD A, B
        case 0x79: regs.A = regs.C; break;         // LD A, C
        case 0x7A: regs.A = regs.D; break;         // LD A, D
        case 0x7B: regs.A = regs.E; break;         // LD A, E
        case 0x7C: regs.A = regs.H; break;         // LD A, H
        case 0x7D: regs.A = regs.L; break;         // LD A, L
        case 0x7E: regs.A = read(regs.HL); break;  // LD A, (HL)
        case 0x7F: break;                          // LD A, A
        // load 0xFF00
        case 0xE0: skd_write(0xFF00 + n(), regs.A); break;  // LDH (n), A
        case 0xF0: skd_read(0xFF00 + n(), regs.A); break;   // LDH A, (n)
        case 0xE2: write(regs.C + 0xFF00, regs.A); break;   // LDH (C), A
        case 0xF2: regs.A = read(regs.C + 0xFF00); break;   // LDH A, (C)
        // 16-bit memory load
        case 0xEA: skd_write(nn(), regs.A); break; // LD (nn), A
        case 0xFA: skd_read(nn(), regs.A); break;  // LD A, (nn)
        // LD reg16, nn
        case 0x01: regs.BC = nn(); break; // LD BC, nn
        case 0x11: regs.DE = nn(); break; // LD DE, nn
        case 0x21: regs.HL = nn(); break; // LD HL, nn
        case 0x31: SP = nn(); break;      // LD SP, nn
        // POP reg16
        case 0xC1: pop(regs.BC); break; // POP BC
        case 0xD1: pop(regs.DE); break; // POP DE
        case 0xE1: pop(regs.HL); break; // POP HL
        case 0xF1:
            regs.F = (read(SP++) & 0xF0) | (regs.F & 0xF);
            skd_read(SP++, regs.A);
            break; // POP AF
        // PUSH reg16
        case 0xC5: push(regs.BC); break; // PUSH BC
        case 0xD5: push(regs.DE); break; // PUSH DE
        case 0xE5: push(regs.HL); break; // PUSH HL
        case 0xF5: push(regs.AF); break; // PUSH AF
        // load SP
        case 0x08: {
            u16 addr = nn();
            write(addr, SP & 0xFF);
            write(addr + 1, SP >> 8);
        } break; // LD (nn), SP
        case 0xF9: cycleCnt++; SP = regs.HL; break; // LD SP, HL
        // ADD A, reg8
        case 0x80: add8(regs.B); break;         // ADD A, B
        case 0x81: add8(regs.C); break;         // ADD A, C
        case 0x82: add8(regs.D); break;         // ADD A, D
        case 0x83: add8(regs.E); break;         // ADD A, E
        case 0x84: add8(regs.H); break;         // ADD A, H
        case 0x85: add8(regs.L); break;         // ADD A, L
        case 0x86: add8(read(regs.HL)); break;  // ADD A, (HL)
        case 0x87: add8(regs.A); break;         // ADD A, A
        case 0xC6: add8(n()); break;            // ADD A, n
        // ADC A, reg8
        case 0x88: adc8(regs.B); break;         // ADC A, B
        case 0x89: adc8(regs.C); break;         // ADC A, C
        case 0x8A: adc8(regs.D); break;         // ADC A, D
        case 0x8B: adc8(regs.E); break;         // ADC A, E
        case 0x8C: adc8(regs.H); break;         // ADC A, H
        case 0x8D: adc8(regs.L); break;         // ADC A, L
        case 0x8E: adc8(read(regs.HL)); break;  // ADC A, (HL)
        case 0x8F: adc8(regs.A); break;         // ADC A, A
        case 0xCE: adc8(n()); break;            // ADC A, n
        // SUB A, reg8
        case 0x90: sub8(regs.B); break;         // SUB B
        case 0x91: sub8(regs.C); break;         // SUB C
        case 0x92: sub8(regs.D); break;         // SUB D
        case 0x93: sub8(regs.E); break;         // SUB E
        case 0x94: sub8(regs.H); break;         // SUB H
        case 0x95: sub8(regs.L); break;         // SUB L
        case 0x96: sub8(read(regs.HL)); break;  // SUB (HL)
        case 0x97: sub8(regs.A); break;         // SUB A
        case 0xD6: sub8(n()); break;            // SUB n
        // SBC A, reg8
        case 0x98: sbc8(regs.B); break;         // SBC A, B
        case 0x99: sbc8(regs.C); break;         // SBC A, C
        case 0x9A: sbc8(regs.D); break;         // SBC A, D
        case 0x9B: sbc8(regs.E); break;         // SBC A, E
        case 0x9C: sbc8(regs.H); break;         // SBC A, H
        case 0x9D: sbc8(regs.L); break;         // SBC A, L
        case 0x9E: sbc8(read(regs.HL)); break;  // SBC A, (HL)
        case 0x9F: sbc8(regs.A); break;         // SBC A, A
        case 0xDE: sbc8(n()); break;            // SBC A, n
        // AND A, reg8
        case 0xA0: and8(regs.B); break;         // AND B
        case 0xA1: and8(regs.C); break;         // AND C
        case 0xA2: and8(regs.D); break;         // AND D
        case 0xA3: and8(regs.E); break;         // AND E
        case 0xA4: and8(regs.H); break;         // AND H
        case 0xA5: and8(regs.L); break;         // AND L
        case 0xA6: and8(read(regs.HL)); break;  // AND (HL)
        case 0xA7: and8(regs.A); break;         // AND A
        case 0xE6: and8(n()); break;            // AND n
        // XOR A, reg8
        case 0xA8: xor8(regs.B); break;         // XOR B
        case 0xA9: xor8(regs.C); break;         // XOR C
        case 0xAA: xor8(regs.D); break;         // XOR D
        case 0xAB: xor8(regs.E); break;         // XOR E
        case 0xAC: xor8(regs.H); break;         // XOR H
        case 0xAD: xor8(regs.L); break;         // XOR L
        case 0xAE: xor8(read(regs.HL)); break;  // XOR (HL)
        case 0xAF: xor8(regs.A); break;         // XOR A
        case 0xEE: xor8(n()); break;            // XOR A, n
        // OR A, reg8
        case 0xB0: or8(regs.B); break;          // OR B
        case 0xB1: or8(regs.C); break;          // OR C
        case 0xB2: or8(regs.D); break;          // OR D
        case 0xB3: or8(regs.E); break;          // OR E
        case 0xB4: or8(regs.H); break;          // OR H
        case 0xB5: or8(regs.L); break;          // OR L
        case 0xB6: or8(read(regs.HL)); break;   // OR (HL)
        case 0xB7: or8(regs.A); break;          // OR A
        case 0xF6: or8(n()); break;             // OR n
        // CP A, reg8
        case 0xB8: cp8(regs.B); break;         // CP B
        case 0xB9: cp8(regs.C); break;         // CP C
        case 0xBA: cp8(regs.D); break;         // CP D
        case 0xBB: cp8(regs.E); break;         // CP E
        case 0xBC: cp8(regs.H); break;         // CP H
        case 0xBD: cp8(regs.L); break;         // CP L
        case 0xBE: cp8(read(regs.HL)); break;  // CP (HL)
        case 0xBF: cp8(regs.A); break;         // CP A
        case 0xFE: cp8(n()); break;            // CP n
        // INC reg8
        case 0x04: inc8(regs.B); break; // INC B
        case 0x0C: inc8(regs.C); break; // INC C
        case 0x14: inc8(regs.D); break; // INC D
        case 0x1C: inc8(regs.E); break; // INC E
        case 0x24: inc8(regs.H); break; // INC H
        case 0x2C: inc8(regs.L); break; // INC L
        case 0x34:
            temp8 = read(regs.HL);
            inc8(temp8);
            skd_write(regs.HL, temp8);
            break; // INC (HL)
        case 0x3C: inc8(regs.A); break; // INC A
        // DEC reg8
        case 0x05: dec8(regs.B); break; // DEC B
        case 0x0D: dec8(regs.C); break; // DEC C
        case 0x15: dec8(regs.D); break; // DEC D
        case 0x1D: dec8(regs.E); break; // DEC E
        case 0x25: dec8(regs.H); break; // DEC H
        case 0x2D: dec8(regs.L); break; // DEC L
        case 0x35:
            temp8 = read(regs.HL);
            dec8(temp8);
            skd_write(regs.HL, temp8);
            break; // DEC (HL)
        case 0x3D: dec8(regs.A); break; // DEC A
        // ADD HL, n
        case 0x09: add16(regs.BC); break; // ADD HL, BC
        case 0x19: add16(regs.DE); break; // ADD HL, DE
        case 0x29: add16(regs.HL); break; // ADD HL, HL
        case 0x39: add16(SP); break;      // ADD HL, SP
        // SP + n
        case 0xE8: cycleCnt++; SP = addSP_n(); break; // ADD SP, n
        case 0xF8: regs.HL = addSP_n(); break;        // LD HL, SP + n
        // INC reg16
        case 0x03: cycleCnt++; regs.BC++; break; // INC BC
        case 0x13: cycleCnt++; regs.DE++; break; // INC DE
        case 0x23: cycleCnt++; regs.HL++; break; // INC HL
        case 0x33: cycleCnt++; SP++; break;      // INC SP
        // DEC reg16
        case 0x0B: cycleCnt++; regs.BC--; break; // DEC BC
        case 0x1B: cycleCnt++; regs.DE--; break; // DEC DE
        case 0x2B: cycleCnt++; regs.HL--; break; // DEC HL
        case 0x3B: cycleCnt++; SP--; break;      // DEC SP
        // Rotate left A
        case 0x07: rlc8(regs.A); set_flag(Z_FLAG, false); break; // RLCA
        case 0x17: rl8(regs.A); set_flag(Z_FLAG, false); break;  // RLA
        // Rotate right A
        case 0x0F: rrc8(regs.A); set_flag(Z_FLAG, false); break; // RRCA
        case 0x1F: rr8(regs.A); set_flag(Z_FLAG, false); break;  // RRA
        // Misc ALU
        case 0x27: daa(); break; // DAA
        case 0x37:
            set_flag(N_FLAG, false);
            set_flag(H_FLAG, false);
            set_flag(C_FLAG, true);
            break; // SCF
        case 0x2F:
            regs.A = ~regs.A;
            set_flag(N_FLAG, true);
            set_flag(H_FLAG, true);
            break; // CPL
        case 0x3F:
            set_flag(N_FLAG, false);
            set_flag(H_FLAG, false);
            set_flag(C_FLAG, !check_flag(C_FLAG));
            break; // CCF
        // JP
        case 0xC3: jump_nn(); break; // JP nn
        case 0xC2:
            if (!check_flag(Z_FLAG)) {
                jump_nn();
            } else {
                nn();
            }
            break; // JP NZ, nn
        case 0xD2:
            if (!check_flag(C_FLAG)) {
                jump_nn();
            } else {
                nn();
            }
            break; // JP NC, nn
        case 0xCA:
            if (check_flag(Z_FLAG)) {
                jump_nn();
            } else {
                nn();
            }
            break; // JP Z, nn
        case 0xDA:
            if (check_flag(C_FLAG)) {
                jump_nn();
            } else {
                nn();
            }
            break; // JP C, nn
        case 0xE9: PC = regs.HL; break; // JP HL
        // JR
        case 0x18: {
            s8 off = (s8)n();
            PC += off;
            cycleCnt++;
        } break; // JR n
        case 0x20: {
            s8 off = (s8)n();
            if (!check_flag(Z_FLAG)) {
                PC += off;
                cycleCnt++;
            }
        } break; // JR NZ, n
        case 0x30: {
            s8 off = (s8)n();
            if (!check_flag(C_FLAG)) {
                PC += off;
                cycleCnt++;
            }
        } break; // JR NC, n
        case 0x28: {
            s8 off = (s8)n();
            if (check_flag(Z_FLAG)) {
                PC += off;
                cycleCnt++;
            }
        } break; // JR Z, n
        case 0x38: {
            s8 off = (s8)n();
            if (check_flag(C_FLAG)) {
                PC += off;
                cycleCnt++;
            }
        } break; // JR C, n
        // CALL
        case 0xCD: call_nn(); break; // CALL
        case 0xC4:
            if (!check_flag(Z_FLAG)) {
                call_nn();
            } else {
                nn();
            }
            break; // CALL NZ, nn
        case 0xD4:
            if (!check_flag(C_FLAG)) {
                call_nn();
            } else {
                nn();
            }
            break; // CALL NC, nn
        case 0xCC:
            if (check_flag(Z_FLAG)) {
                call_nn();
            } else {
                nn();
            }
            break; // CALL Z, nn
        case 0xDC:
            if (check_flag(C_FLAG)) {
                call_nn();
            } else {
                nn();
            }
            break; // CALL C, nn
        // RST
        case 0xC7: rst(0x00); break; // RST 00H
        case 0xCF: rst(0x08); break; // RST 08H
        case 0xD7: rst(0x10); break; // RST 10H
        case 0xDF: rst(0x18); break; // RST 18H
        case 0xE7: rst(0x20); break; // RST 20H
        case 0xEF: rst(0x28); break; // RST 28H
        case 0xF7: rst(0x30); break; // RST 30H
        case 0xFF: rst(0x38); break; // RST 38H
        // RET
        case 0xC9: pop(PC); cycleCnt++; break; // RET
        case 0xC0:
            cycleCnt++;
            if (!check_flag(Z_FLAG)) {
                pop(PC);
                cycleCnt++;
            }
            break; // RET NZ
        case 0xD0:
            cycleCnt++;
            if (!check_flag(C_FLAG)) {
                pop(PC);
                cycleCnt++;
            }
            break; // RET NC
        case 0xC8:
            cycleCnt++;
            if (check_flag(Z_FLAG)) {
                pop(PC);
                cycleCnt++;
            }
            break; // RET Z
        case 0xD8:
            cycleCnt++;
            if (check_flag(C_FLAG)) {
                pop(PC);
                cycleCnt++;
            }
            break; // RET C
        case 0xD9: pop(PC); cycleCnt++; ime = true; break; // RETI
        // CB
        case 0xCB: execute_cb(); break;
        // Error
        case 0xD3:
        case 0xE3:
        case 0xE4:
        case 0xF4:
        case 0xDB:
        case 0xEB:
        case 0xEC:
        case 0xFC:
        case 0xDD:
        case 0xED:
        case 0xFD:
            freeze();
            break;
    }
}

void CPU::execute_cb() {
    u8 cbOP = n();
    if (cbOP < 0x40) {
        switch (cbOP) {
            // RLC reg8
            case 0x00: rlc8(regs.B); break; // RLC B
            case 0x01: rlc8(regs.C); break; // RLC C
            case 0x02: rlc8(regs.D); break; // RLC D
            case 0x03: rlc8(regs.E); break; // RLC E
            case 0x04: rlc8(regs.H); break; // RLC H
            case 0x05: rlc8(regs.L); break; // RLC L
            case 0x06:
                skd_read(regs.HL, temp8, [](CPU* c) { c->rlc8(c->temp8); });
                skd_write(regs.HL, temp8);
                break; // RLC (HL)
            case 0x07: rlc8(regs.A); break; // RLC A
            // RRC reg8
            case 0x08: rrc8(regs.B); break; // RRC B
            case 0x09: rrc8(regs.C); break; // RRC C
            case 0x0A: rrc8(regs.D); break; // RRC D
            case 0x0B: rrc8(regs.E); break; // RRC E
            case 0x0C: rrc8(regs.H); break; // RRC H
            case 0x0D: rrc8(regs.L); break; // RRC L
            case 0x0E:
                skd_read(regs.HL, temp8, [](CPU* c) { c->rrc8(c->temp8); });
                skd_write(regs.HL, temp8);
                break;  // RRC (HL)
            case 0x0F: rrc8(regs.A); break; // RRC A
            // RL reg8
            case 0x10: rl8(regs.B); break; // RL B
            case 0x11: rl8(regs.C); break; // RL C
            case 0x12: rl8(regs.D); break; // RL D
            case 0x13: rl8(regs.E); break; // RL E
            case 0x14: rl8(regs.H); break; // RL H
            case 0x15: rl8(regs.L); break; // RL L
            case 0x16:
                skd_read(regs.HL, temp8, [](CPU* c) { c->rl8(c->temp8); });
                skd_write(regs.HL, temp8);
                break; // RL (HL)
            case 0x17: rl8(regs.A); break; // RL A
            // RR reg8
            case 0x18: rr8(regs.B); break; // RR B
            case 0x19: rr8(regs.C); break; // RR C
            case 0x1A: rr8(regs.D); break; // RR D
            case 0x1B: rr8(regs.E); break; // RR E
            case 0x1C: rr8(regs.H); break; // RR H
            case 0x1D: rr8(regs.L); break; // RR L
            case 0x1E:
                skd_read(regs.HL, temp8, [](CPU* c) { c->rr8(c->temp8); });
                skd_write(regs.HL, temp8);
                break; // RR (HL)
            case 0x1F: rr8(regs.A); break; // RR A
            // SLA reg8
            case 0x20: sla8(regs.B); break; // SLA B
            case 0x21: sla8(regs.C); break; // SLA C
            case 0x22: sla8(regs.D); break; // SLA D
            case 0x23: sla8(regs.E); break; // SLA E
            case 0x24: sla8(regs.H); break; // SLA H
            case 0x25: sla8(regs.L); break; // SLA L
            case 0x26:
                skd_read(regs.HL, temp8, [](CPU* c) { c->sla8(c->temp8); });
                skd_write(regs.HL, temp8);
                break; // SLA (HL)
            case 0x27: sla8(regs.A); break; // SLA A
            // SRA reg8
            case 0x28: sra8(regs.B); break; // SRA B
            case 0x29: sra8(regs.C); break; // SRA C
            case 0x2A: sra8(regs.D); break; // SRA D
            case 0x2B: sra8(regs.E); break; // SRA E
            case 0x2C: sra8(regs.H); break; // SRA H
            case 0x2D: sra8(regs.L); break; // SRA L
            case 0x2E:
                skd_read(regs.HL, temp8, [](CPU* c) { c->sra8(c->temp8); });
                skd_write(regs.HL, temp8);
                break; // SRA (HL)
            case 0x2F: sra8(regs.A); break; // SRA A
            // SWAP reg8
            case 0x30: swap(regs.B); break; // SWAP B
            case 0x31: swap(regs.C); break; // SWAP C
            case 0x32: swap(regs.D); break; // SWAP D
            case 0x33: swap(regs.E); break; // SWAP E
            case 0x34: swap(regs.H); break; // SWAP H
            case 0x35: swap(regs.L); break; // SWAP L
            case 0x36:
                skd_read(regs.HL, temp8, [](CPU* c) { c->swap(c->temp8); });
                skd_write(regs.HL, temp8);
                break; // SWAP (HL)
            case 0x37: swap(regs.A); break; // SWAP A
            // SRL reg8
            case 0x38: srl8(regs.B); break; // SRL B
            case 0x39: srl8(regs.C); break; // SRL C
            case 0x3A: srl8(regs.D); break; // SRL D
            case 0x3B: srl8(regs.E); break; // SRL E
            case 0x3C: srl8(regs.H); break; // SRL H
            case 0x3D: srl8(regs.L); break; // SRL L
            case 0x3E:
                skd_read(regs.HL, temp8, [](CPU* c) { c->srl8(c->temp8); });
                skd_write(regs.HL, temp8);
                break; // SRL (HL)
            case 0x3F: srl8(regs.A); break; // SRL A
            default:
                fatal("Impossible cb opcode=%02x\n", cbOP);
        }
    } else {
        bitReg = (cbOP / 0x8) % 0x8;
        u8 index = ((cbOP / 0x40) - 1) * 0x8 + (cbOP % 0x8);
        switch (index) {
            // BIT
            case 0x00: bit(regs.B); break; // BIT b, B
            case 0x01: bit(regs.C); break; // BIT b, C
            case 0x02: bit(regs.D); break; // BIT b, D
            case 0x03: bit(regs.E); break; // BIT b, E
            case 0x04: bit(regs.H); break; // BIT b, H
            case 0x05: bit(regs.L); break; // BIT b, L
            case 0x06: skd_read(regs.HL, temp8, [](CPU* c) { c->bit(c->temp8); }); break; // BIT b, (HL)
            case 0x07: bit(regs.A); break; // BIT b, A
            // RES
            case 0x08: res(regs.B); break; // RES b, B
            case 0x09: res(regs.C); break; // RES b, C
            case 0x0A: res(regs.D); break; // RES b, D
            case 0x0B: res(regs.E); break; // RES b, E
            case 0x0C: res(regs.H); break; // RES b, H
            case 0x0D: res(regs.L); break; // RES b, L
            case 0x0E:
                skd_read(regs.HL, temp8, [](CPU* c) { c->res(c->temp8); });
                skd_write(regs.HL, temp8);
                break; // RES b, (HL)
            case 0x0F: res(regs.A); break; // RES b, A
            // SET
            case 0x10: set(regs.B); break; // SET b, B
            case 0x11: set(regs.C); break; // SET b, C
            case 0x12: set(regs.D); break; // SET b, D
            case 0x13: set(regs.E); break; // SET b, E
            case 0x14: set(regs.H); break; // SET b, H
            case 0x15: set(regs.L); break; // SET b, L
            case 0x16:
                skd_read(regs.HL, temp8, [](CPU* c) { c->set(c->temp8); });
                skd_write(regs.HL, temp8);
                break; // SET b, (HL)
            case 0x17: set(regs.A); break; // SET b, A
            default:
                fatal("Impossible bit operation for cb opcode=%02x\n", cbOP);
        }
    }
}
// clang-format on

void CPU::set_flag(u8 flag, bool set) { regs.F = (regs.F & ~flag) | (flag * set); }
bool CPU::check_flag(u8 flag) { return (regs.F & flag) != 0; }
u8 CPU::n() { return read(PC++); }
u16 CPU::nn() {
    u16 res = read(PC);
    PC++;
    res |= read(PC) << 8;
    PC++;
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
void CPU::skd_read(u16 addr, u8& dest, Callback&& readCallback) {
    skd_read(addr, dest);
    callbackCycle = cycleCnt;
    callback = readCallback;
}
void CPU::skd_read(u16 addr, u8& dest) {
    memory->schedule_read(addr, &dest, cycleCnt);
    cycleCnt++;
}
void CPU::skd_write(u16 addr, u8& val) {
    memory->schedule_write(addr, &val, cycleCnt);
    cycleCnt++;
}

void CPU::pop(u16& dest) {
    skd_read(SP++, *((u8*)&dest));
    skd_read(SP++, *((u8*)&dest + 1));
}

void CPU::push(u16 reg) {
    cycleCnt++;  // Stack push takes an extra blank cycle before pushing

    static u8 msb;
    static u8 lsb;
    msb = reg >> 8;
    lsb = reg & 0xFF;
    skd_write(--SP, msb);
    skd_write(--SP, lsb);
}

void CPU::add8(u8 val) {
    u8 res = regs.A + val;
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, (regs.A & 0xF) + (val & 0xF) > 0xF);
    set_flag(C_FLAG, (0xFF - regs.A) < val);
    regs.A = res;
}

void CPU::adc8(u8 val) {
    u8 carry = check_flag(C_FLAG);
    u8 res = regs.A + val + carry;
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, carry + (regs.A & 0xF) + (val & 0xF) > 0xF);
    set_flag(C_FLAG, val + carry > 0xFF - regs.A);
    regs.A = res;
}

void CPU::sub8(u8 val) {
    u8 res = regs.A - val;
    set_flag(Z_FLAG, regs.A == val);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (regs.A & 0xF) < (val & 0xF));
    set_flag(C_FLAG, regs.A < val);
    regs.A = res;
}

void CPU::sbc8(u8 val) {
    u8 carry = check_flag(C_FLAG);
    u8 res = regs.A - (val + carry);
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (regs.A & 0xF) < ((val & 0xF) + carry));
    set_flag(C_FLAG, regs.A < val + carry);
    regs.A = res;
}

void CPU::and8(u8 val) {
    regs.A &= val;
    set_flag(Z_FLAG, regs.A == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, true);
    set_flag(C_FLAG, false);
}

void CPU::xor8(u8 val) {
    regs.A ^= val;
    set_flag(Z_FLAG, regs.A == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, false);
}

void CPU::or8(u8 val) {
    regs.A |= val;
    set_flag(Z_FLAG, regs.A == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, false);
}

void CPU::cp8(u8 val) {
    set_flag(Z_FLAG, regs.A == val);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (regs.A & 0xF) < (val & 0xF));
    set_flag(C_FLAG, regs.A < val);
}

void CPU::inc8(u8& reg) {
    reg++;
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, !(reg & 0xF));
}

void CPU::dec8(u8& reg) {
    reg--;
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (reg & 0xF) == 0xF);
}

void CPU::add16(u16 val) {
    u16 res = regs.HL + val;
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, (0xFFF - (regs.HL & 0xFFF)) < (val & 0xFFF));
    set_flag(C_FLAG, (0xFFFF - regs.HL) < val);
    regs.HL = res;
    cycleCnt++;
}

u16 CPU::addSP_n() {
    u16 stackPtr = SP;
    s8 off = (s8)n();
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

void CPU::daa() {
    if (check_flag(N_FLAG)) {  // Subtraction
        if (check_flag(C_FLAG)) regs.A -= 0x6 << 4;
        if (check_flag(H_FLAG)) regs.A -= 0x6;
    } else {  // Addition
        if (check_flag(C_FLAG) || regs.A > 0x99) {
            regs.A += 0x6 << 4;
            set_flag(C_FLAG, true);
        }
        if (check_flag(H_FLAG) || (regs.A & 0xF) > 0x9) regs.A += 0x6;
    }
    set_flag(Z_FLAG, regs.A == 0);
    set_flag(H_FLAG, false);
}

void CPU::jump_nn() {
    skd_read(PC, *((u8*)&PC));
    PC++;
    skd_read(PC, *((u8*)&PC + 1));
    PC++;
    cycleCnt++;
}

void CPU::call_nn() {
    skd_read(PC, *((u8*)&PC));
    PC++;
    skd_read(PC, *((u8*)&PC + 1));
    PC++;
    push(PC);
}

void CPU::rst(u16 addr) {
    push(PC);
    PC = addr;
}

void CPU::freeze() {
    printf("Invalid opcode, freezing...\n");
    debugger->pause_exec();
}

void CPU::rlc8(u8& reg) {
    set_flag(C_FLAG, reg >> 7);
    reg = (reg << 1) | (reg >> 7);
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}

void CPU::rrc8(u8& reg) {
    set_flag(C_FLAG, reg & 0x1);
    reg = (reg << 7) | (reg >> 1);
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}

void CPU::rl8(u8& reg) {
    u8 res = (reg << 1) | check_flag(C_FLAG);

    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, reg & (1 << 7));

    reg = res;
}

void CPU::rr8(u8& reg) {
    u8 res = (check_flag(C_FLAG) << 7) | (reg >> 1);
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, reg & 0x1);
    reg = res;
}

void CPU::sla8(u8& reg) {
    set_flag(C_FLAG, reg >> 7);
    reg <<= 1;
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}

void CPU::sra8(u8& reg) {
    set_flag(C_FLAG, reg & 0x1);
    reg = (u8)((s8)reg >> 1);  // Arithmetic right shift
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}

void CPU::swap(u8& reg) {
    reg = ((reg & 0xF) << 4) | (reg >> 4);
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, false);
}

void CPU::srl8(u8& reg) {
    set_flag(C_FLAG, reg & 0x1);
    reg >>= 1;
    set_flag(Z_FLAG, reg == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
}

void CPU::bit(u8 reg) {
    set_flag(Z_FLAG, (reg & (1 << bitReg)) == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, true);
}
void CPU::res(u8& reg) { reg &= ~(1 << bitReg); }
void CPU::set(u8& reg) { reg |= 1 << bitReg; }
