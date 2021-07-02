#include "cpu.hpp"

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

    callbackCycle = 0;
}

void CPU::handle_interrupts() {
    // TODO handle weird push to IE case
    u8 ifReg = memory->read(IOReg::IF_REG);
    u8 interrupts = ifReg & memory->read(IOReg::IE_REG) & 0x1F;
    if (interrupts) {
        // TODO only add extra cycle if emulating CGB
        // if (halted) cycleCnt++;  // Extra cycle if cpu is in halt mode

        if (ime) {
            for (int i = 0; i < 5; i++) {
                if (interrupts & (1 << i)) {
                    push(PC);
                    PC = i * 0x8 + 0x40;

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
        // PAUSE_EXEC(this, debugger, 0x47F3);
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
        case 0x02: write(BC.pair, AF.hi); break;   // LD (BC), A
        case 0x12: write(DE.pair, AF.hi); break;   // LD (DE), A
        case 0x22: write(HL.pair++, AF.hi); break; // LD (HL+), A
        case 0x32: write(HL.pair--, AF.hi); break; // LD (HL-), A
        // LD reg8, n
        case 0x06: BC.hi = n(); break; // LD B, n
        case 0x16: DE.hi = n(); break; // LD D, n
        case 0x26: HL.hi = n(); break; // LD H, n
        case 0x36:
            temp8 = n();
            skd_write(HL.pair, temp8);
            break; // LD (HL), n
        case 0x0E: BC.lo = n(); break; // LD C, n
        case 0x1E: DE.lo = n(); break; // LD E, n
        case 0x2E: HL.lo = n(); break; // LD L, n
        case 0x3E: AF.hi = n(); break; // LD A, n
        // LD A, (reg16)
        case 0x0A: AF.hi = read(BC.pair); break;   // LD A, (BC)
        case 0x1A: AF.hi = read(DE.pair); break;   // LD A, (DE)
        case 0x2A: AF.hi = read(HL.pair++); break; // LD A, (HL+)
        case 0x3A: AF.hi = read(HL.pair--); break; // LD A, (HL-)
        // LD B, r2
        case 0x40:
            #if !PLAYABLE
            debugger->pause_exec();
            #endif
            break; // LD B, B
        case 0x41: BC.hi = BC.lo; break;         // LD B, C
        case 0x42: BC.hi = DE.hi; break;         // LD B, D
        case 0x43: BC.hi = DE.lo; break;         // LD B, E
        case 0x44: BC.hi = HL.hi; break;         // LD B, H
        case 0x45: BC.hi = HL.lo; break;         // LD B, L
        case 0x46: BC.hi = read(HL.pair); break; // LD B, (HL)
        case 0x47: BC.hi = AF.hi; break;         // LD B, A
        // LD C, r2
        case 0x48: BC.lo = BC.hi; break;         // LD C, B
        case 0x49: break;                        // LD C, C
        case 0x4A: BC.lo = DE.hi; break;         // LD C, D
        case 0x4B: BC.lo = DE.lo; break;         // LD C, E
        case 0x4C: BC.lo = HL.hi; break;         // LD C, H
        case 0x4D: BC.lo = HL.lo; break;         // LD C, L
        case 0x4E: BC.lo = read(HL.pair); break; // LD C, (HL)
        case 0x4F: BC.lo = AF.hi; break;         // LD C, A
        // LD D, r2
        case 0x50: DE.hi = BC.hi; break;         // LD D, B
        case 0x51: DE.hi = BC.lo; break;         // LD D, C
        case 0x52: break;                        // LD D, D
        case 0x53: DE.hi = DE.lo; break;         // LD D, E
        case 0x54: DE.hi = HL.hi; break;         // LD D, H
        case 0x55: DE.hi = HL.lo; break;         // LD D, L
        case 0x56: DE.hi = read(HL.pair); break; // LD D, (HL)
        case 0x57: DE.hi = AF.hi; break;         // LD D, A
        // LD E, r2
        case 0x58: DE.lo = BC.hi; break;         // LD E, B
        case 0x59: DE.lo = BC.lo; break;         // LD E, C
        case 0x5A: DE.lo = DE.hi; break;         // LD E, D
        case 0x5B: break;                        // LD E, E
        case 0x5C: DE.lo = HL.hi; break;         // LD E, H
        case 0x5D: DE.lo = HL.lo; break;         // LD E, L
        case 0x5E: DE.lo = read(HL.pair); break; // LD E, (HL)
        case 0x5F: DE.lo = AF.hi; break;         // LD E, A
        // LD H, r2
        case 0x60: HL.hi = BC.hi; break;         // LD H, B
        case 0x61: HL.hi = BC.lo; break;         // LD H, C
        case 0x62: HL.hi = DE.hi; break;         // LD H, D
        case 0x63: HL.hi = DE.lo; break;         // LD H, E
        case 0x64: break;                        // LD H, H
        case 0x65: HL.hi = HL.lo; break;         // LD H, L
        case 0x66: HL.hi = read(HL.pair); break; // LD H, (HL)
        case 0x67: HL.hi = AF.hi; break;         // LD H, A
        // LD L, r2
        case 0x68: HL.lo = BC.hi; break;         // LD L, B
        case 0x69: HL.lo = BC.lo; break;         // LD L, C
        case 0x6A: HL.lo = DE.hi; break;         // LD L, D
        case 0x6B: HL.lo = DE.lo; break;         // LD L, E
        case 0x6C: HL.lo = HL.hi; break;         // LD L, H
        case 0x6D: break;                        // LD L, L
        case 0x6E: HL.lo = read(HL.pair); break; // LD L, (HL)
        case 0x6F: HL.lo = AF.hi; break;         // LD L, A
        // LD (HL), r2
        case 0x70: write(HL.pair, BC.hi); break; // LD (HL), B
        case 0x71: write(HL.pair, BC.lo); break; // LD (HL), C
        case 0x72: write(HL.pair, DE.hi); break; // LD (HL), D
        case 0x73: write(HL.pair, DE.lo); break; // LD (HL), E
        case 0x74: write(HL.pair, HL.hi); break; // LD (HL), H
        case 0x75: write(HL.pair, HL.lo); break; // LD (HL), L
        case 0x77: write(HL.pair, AF.hi); break; // LD (HL), A
        // LD A, r2
        case 0x78: AF.hi = BC.hi; break;         // LD A, B
        case 0x79: AF.hi = BC.lo; break;         // LD A, C
        case 0x7A: AF.hi = DE.hi; break;         // LD A, D
        case 0x7B: AF.hi = DE.lo; break;         // LD A, E
        case 0x7C: AF.hi = HL.hi; break;         // LD A, H
        case 0x7D: AF.hi = HL.lo; break;         // LD A, L
        case 0x7E: AF.hi = read(HL.pair); break; // LD A, (HL)
        case 0x7F: break;                        // LD A, A
        // load 0xFF00
        case 0xE0: skd_write(0xFF00 + n(), AF.hi); break; // LDH (n), A
        case 0xF0: skd_read(0xFF00 + n(), AF.hi); break;  // LDH A, (n)
        case 0xE2: write(BC.lo + 0xFF00, AF.hi); break;   // LDH (C), A
        case 0xF2: AF.hi = read(BC.lo + 0xFF00); break;   // LDH A, (C)
        // 16-bit memory load
        case 0xEA: skd_write(nn(), AF.hi); break; // LD (nn), A
        case 0xFA: skd_read(nn(), AF.hi); break;  // LD A, (nn)
        // LD reg16, nn
        case 0x01: BC.pair = nn(); break; // LD BC, nn
        case 0x11: DE.pair = nn(); break; // LD DE, nn
        case 0x21: HL.pair = nn(); break; // LD HL, nn
        case 0x31: SP = nn(); break;      // LD SP, nn
        // POP reg16
        case 0xC1: pop(BC.pair); break; // POP BC
        case 0xD1: pop(DE.pair); break; // POP DE
        case 0xE1: pop(HL.pair); break; // POP HL
        case 0xF1:
            AF.lo = (read(SP++) & 0xF0) | (AF.lo & 0xF);
            skd_read(SP++, AF.hi);
            break; // POP AF
        // PUSH reg16
        case 0xC5: push(BC.pair); break; // PUSH BC
        case 0xD5: push(DE.pair); break; // PUSH DE
        case 0xE5: push(HL.pair); break; // PUSH HL
        case 0xF5: push(AF.pair); break; // PUSH AF
        // load SP
        case 0x08: {
            u16 addr = nn();
            write(addr, SP & 0xFF);
            write(addr + 1, SP >> 8);
        } break; // LD (nn), SP
        case 0xF9: cycleCnt++; SP = HL.pair; break; // LD SP, HL
        // ADD A, reg8
        case 0x80: add8(BC.hi); break;         // ADD A, B
        case 0x81: add8(BC.lo); break;         // ADD A, C
        case 0x82: add8(DE.hi); break;         // ADD A, D
        case 0x83: add8(DE.lo); break;         // ADD A, E
        case 0x84: add8(HL.hi); break;         // ADD A, H
        case 0x85: add8(HL.lo); break;         // ADD A, L
        case 0x86: add8(read(HL.pair)); break; // ADD A, (HL)
        case 0x87: add8(AF.hi); break;         // ADD A, A
        case 0xC6: add8(n()); break;           // ADD A, n
        // ADC A, reg8
        case 0x88: adc8(BC.hi); break;         // ADC A, B
        case 0x89: adc8(BC.lo); break;         // ADC A, C
        case 0x8A: adc8(DE.hi); break;         // ADC A, D
        case 0x8B: adc8(DE.lo); break;         // ADC A, E
        case 0x8C: adc8(HL.hi); break;         // ADC A, H
        case 0x8D: adc8(HL.lo); break;         // ADC A, L
        case 0x8E: adc8(read(HL.pair)); break; // ADC A, (HL)
        case 0x8F: adc8(AF.hi); break;         // ADC A, A
        case 0xCE: adc8(n()); break;           // ADC A, n
        // SUB A, reg8
        case 0x90: sub8(BC.hi); break;         // SUB B
        case 0x91: sub8(BC.lo); break;         // SUB C
        case 0x92: sub8(DE.hi); break;         // SUB D
        case 0x93: sub8(DE.lo); break;         // SUB E
        case 0x94: sub8(HL.hi); break;         // SUB H
        case 0x95: sub8(HL.lo); break;         // SUB L
        case 0x96: sub8(read(HL.pair)); break; // SUB (HL)
        case 0x97: sub8(AF.hi); break;         // SUB A
        case 0xD6: sub8(n()); break;           // SUB n
        // SBC A, reg8
        case 0x98: sbc8(BC.hi); break;         // SBC A, B
        case 0x99: sbc8(BC.lo); break;         // SBC A, C
        case 0x9A: sbc8(DE.hi); break;         // SBC A, D
        case 0x9B: sbc8(DE.lo); break;         // SBC A, E
        case 0x9C: sbc8(HL.hi); break;         // SBC A, H
        case 0x9D: sbc8(HL.lo); break;         // SBC A, L
        case 0x9E: sbc8(read(HL.pair)); break; // SBC A, (HL)
        case 0x9F: sbc8(AF.hi); break;         // SBC A, A
        case 0xDE: sbc8(n()); break;           // SBC A, n
        // AND A, reg8
        case 0xA0: and8(BC.hi); break;         // AND B
        case 0xA1: and8(BC.lo); break;         // AND C
        case 0xA2: and8(DE.hi); break;         // AND D
        case 0xA3: and8(DE.lo); break;         // AND E
        case 0xA4: and8(HL.hi); break;         // AND H
        case 0xA5: and8(HL.lo); break;         // AND L
        case 0xA6: and8(read(HL.pair)); break; // AND (HL)
        case 0xA7: and8(AF.hi); break;         // AND A
        case 0xE6: and8(n()); break;           // AND n
        // XOR A, reg8
        case 0xA8: xor8(BC.hi); break;         // XOR B
        case 0xA9: xor8(BC.lo); break;         // XOR C
        case 0xAA: xor8(DE.hi); break;         // XOR D
        case 0xAB: xor8(DE.lo); break;         // XOR E
        case 0xAC: xor8(HL.hi); break;         // XOR H
        case 0xAD: xor8(HL.lo); break;         // XOR L
        case 0xAE: xor8(read(HL.pair)); break; // XOR (HL)
        case 0xAF: xor8(AF.hi); break;         // XOR A
        case 0xEE: xor8(n()); break;           // XOR A, n
        // OR A, reg8
        case 0xB0: or8(BC.hi); break;          // OR B
        case 0xB1: or8(BC.lo); break;          // OR C
        case 0xB2: or8(DE.hi); break;          // OR D
        case 0xB3: or8(DE.lo); break;          // OR E
        case 0xB4: or8(HL.hi); break;          // OR H
        case 0xB5: or8(HL.lo); break;          // OR L
        case 0xB6: or8(read(HL.pair)); break;  // OR (HL)
        case 0xB7: or8(AF.hi); break;          // OR A
        case 0xF6: or8(n()); break;            // OR n
        // CP A, reg8
        case 0xB8: cp8(BC.hi); break;         // CP B
        case 0xB9: cp8(BC.lo); break;         // CP C
        case 0xBA: cp8(DE.hi); break;         // CP D
        case 0xBB: cp8(DE.lo); break;         // CP E
        case 0xBC: cp8(HL.hi); break;         // CP H
        case 0xBD: cp8(HL.lo); break;         // CP L
        case 0xBE: cp8(read(HL.pair)); break; // CP (HL)
        case 0xBF: cp8(AF.hi); break;         // CP A
        case 0xFE: cp8(n()); break;           // CP n
        // INC reg8
        case 0x04: inc8(BC.hi); break; // INC B
        case 0x0C: inc8(BC.lo); break; // INC C
        case 0x14: inc8(DE.hi); break; // INC D
        case 0x1C: inc8(DE.lo); break; // INC E
        case 0x24: inc8(HL.hi); break; // INC H
        case 0x2C: inc8(HL.lo); break; // INC L
        case 0x34:
            temp8 = read(HL.pair);
            inc8(temp8);
            skd_write(HL.pair, temp8);
            break; // INC (HL)
        case 0x3C: inc8(AF.hi); break; // INC A
        // DEC reg8
        case 0x05: dec8(BC.hi); break; // DEC B
        case 0x0D: dec8(BC.lo); break; // DEC C
        case 0x15: dec8(DE.hi); break; // DEC D
        case 0x1D: dec8(DE.lo); break; // DEC E
        case 0x25: dec8(HL.hi); break; // DEC H
        case 0x2D: dec8(HL.lo); break; // DEC L
        case 0x35:
            temp8 = read(HL.pair);
            dec8(temp8);
            skd_write(HL.pair, temp8);
            break; // DEC (HL)
        case 0x3D: dec8(AF.hi); break; // DEC A
        // ADD HL, n
        case 0x09: add16(BC.pair); break; // ADD HL, BC
        case 0x19: add16(DE.pair); break; // ADD HL, DE
        case 0x29: add16(HL.pair); break; // ADD HL, HL
        case 0x39: add16(SP); break;      // ADD HL, SP
        // SP + n
        case 0xE8: cycleCnt++; SP = addSP_n(); break; // ADD SP, n
        case 0xF8: HL.pair = addSP_n(); break;        // LD HL, SP + n
        // INC reg16
        case 0x03: cycleCnt++; BC.pair++; break; // INC BC
        case 0x13: cycleCnt++; DE.pair++; break; // INC DE
        case 0x23: cycleCnt++; HL.pair++; break; // INC HL
        case 0x33: cycleCnt++; SP++; break;      // INC SP
        // DEC reg16
        case 0x0B: cycleCnt++; BC.pair--; break; // DEC BC
        case 0x1B: cycleCnt++; DE.pair--; break; // DEC DE
        case 0x2B: cycleCnt++; HL.pair--; break; // DEC HL
        case 0x3B: cycleCnt++; SP--; break;      // DEC SP
        // Rotate left A
        case 0x07: rlc8(AF.hi); set_flag(Z_FLAG, false); break; // RLCA
        case 0x17: rl8(AF.hi); set_flag(Z_FLAG, false); break;  // RLA
        // Rotate right A
        case 0x0F: rrc8(AF.hi); set_flag(Z_FLAG, false); break; // RRCA
        case 0x1F: rr8(AF.hi); set_flag(Z_FLAG, false); break;  // RRA
        // Misc ALU
        case 0x27: daa(); break; // DAA
        case 0x37:
            set_flag(N_FLAG, false);
            set_flag(H_FLAG, false);
            set_flag(C_FLAG, true);
            break; // SCF
        case 0x2F:
            AF.hi = ~AF.hi;
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
        case 0xE9: PC = HL.pair; break; // JP HL
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
            case 0x00: rlc8(BC.hi); break; // RLC B
            case 0x01: rlc8(BC.lo); break; // RLC C
            case 0x02: rlc8(DE.hi); break; // RLC D
            case 0x03: rlc8(DE.lo); break; // RLC E
            case 0x04: rlc8(HL.hi); break; // RLC H
            case 0x05: rlc8(HL.lo); break; // RLC L
            case 0x06:
                skd_read(HL.pair, temp8, [](CPU* c) { c->rlc8(c->temp8); });
                skd_write(HL.pair, temp8);
                break; // RLC (HL)
            case 0x07: rlc8(AF.hi); break; // RLC A
            // RRC reg8
            case 0x08: rrc8(BC.hi); break; // RRC B
            case 0x09: rrc8(BC.lo); break; // RRC C
            case 0x0A: rrc8(DE.hi); break; // RRC D
            case 0x0B: rrc8(DE.lo); break; // RRC E
            case 0x0C: rrc8(HL.hi); break; // RRC H
            case 0x0D: rrc8(HL.lo); break; // RRC L
            case 0x0E:
                skd_read(HL.pair, temp8, [](CPU* c) { c->rrc8(c->temp8); });
                skd_write(HL.pair, temp8);
                break;  // RRC (HL)
            case 0x0F: rrc8(AF.hi); break; // RRC A
            // RL reg8
            case 0x10: rl8(BC.hi); break; // RL B
            case 0x11: rl8(BC.lo); break; // RL C
            case 0x12: rl8(DE.hi); break; // RL D
            case 0x13: rl8(DE.lo); break; // RL E
            case 0x14: rl8(HL.hi); break; // RL H
            case 0x15: rl8(HL.lo); break; // RL L
            case 0x16:
                skd_read(HL.pair, temp8, [](CPU* c) { c->rl8(c->temp8); });
                skd_write(HL.pair, temp8);
                break; // RL (HL)
            case 0x17: rl8(AF.hi); break; // RL A
            // RR reg8
            case 0x18: rr8(BC.hi); break; // RR B
            case 0x19: rr8(BC.lo); break; // RR C
            case 0x1A: rr8(DE.hi); break; // RR D
            case 0x1B: rr8(DE.lo); break; // RR E
            case 0x1C: rr8(HL.hi); break; // RR H
            case 0x1D: rr8(HL.lo); break; // RR L
            case 0x1E:
                skd_read(HL.pair, temp8, [](CPU* c) { c->rr8(c->temp8); });
                skd_write(HL.pair, temp8);
                break; // RR (HL)
            case 0x1F: rr8(AF.hi); break; // RR A
            // SLA reg8
            case 0x20: sla8(BC.hi); break; // SLA B
            case 0x21: sla8(BC.lo); break; // SLA C
            case 0x22: sla8(DE.hi); break; // SLA D
            case 0x23: sla8(DE.lo); break; // SLA E
            case 0x24: sla8(HL.hi); break; // SLA H
            case 0x25: sla8(HL.lo); break; // SLA L
            case 0x26:
                skd_read(HL.pair, temp8, [](CPU* c) { c->sla8(c->temp8); });
                skd_write(HL.pair, temp8);
                break; // SLA (HL)
            case 0x27: sla8(AF.hi); break; // SLA A
            // SRA reg8
            case 0x28: sra8(BC.hi); break; // SRA B
            case 0x29: sra8(BC.lo); break; // SRA C
            case 0x2A: sra8(DE.hi); break; // SRA D
            case 0x2B: sra8(DE.lo); break; // SRA E
            case 0x2C: sra8(HL.hi); break; // SRA H
            case 0x2D: sra8(HL.lo); break; // SRA L
            case 0x2E:
                skd_read(HL.pair, temp8, [](CPU* c) { c->sra8(c->temp8); });
                skd_write(HL.pair, temp8);
                break; // SRA (HL)
            case 0x2F: sra8(AF.hi); break; // SRA A
            // SWAP reg8
            case 0x30: swap(BC.hi); break; // SWAP B
            case 0x31: swap(BC.lo); break; // SWAP C
            case 0x32: swap(DE.hi); break; // SWAP D
            case 0x33: swap(DE.lo); break; // SWAP E
            case 0x34: swap(HL.hi); break; // SWAP H
            case 0x35: swap(HL.lo); break; // SWAP L
            case 0x36:
                skd_read(HL.pair, temp8, [](CPU* c) { c->swap(c->temp8); });
                skd_write(HL.pair, temp8);
                break; // SWAP (HL)
            case 0x37: swap(AF.hi); break; // SWAP A
            // SRL reg8
            case 0x38: srl8(BC.hi); break; // SRL B
            case 0x39: srl8(BC.lo); break; // SRL C
            case 0x3A: srl8(DE.hi); break; // SRL D
            case 0x3B: srl8(DE.lo); break; // SRL E
            case 0x3C: srl8(HL.hi); break; // SRL H
            case 0x3D: srl8(HL.lo); break; // SRL L
            case 0x3E:
                skd_read(HL.pair, temp8, [](CPU* c) { c->srl8(c->temp8); });
                skd_write(HL.pair, temp8);
                break; // SRL (HL)
            case 0x3F: srl8(AF.hi); break; // SRL A
            default:
                fatal("Impossible cb opcode=%02x\n", cbOP);
        }
    } else {
        bitReg = (cbOP / 0x8) % 0x8;
        u8 index = ((cbOP / 0x40) - 1) * 0x8 + (cbOP % 0x8);
        switch (index) {
            // BIT
            case 0x00: bit(BC.hi); break; // BIT b, B
            case 0x01: bit(BC.lo); break; // BIT b, C
            case 0x02: bit(DE.hi); break; // BIT b, D
            case 0x03: bit(DE.lo); break; // BIT b, E
            case 0x04: bit(HL.hi); break; // BIT b, H
            case 0x05: bit(HL.lo); break; // BIT b, L
            case 0x06: skd_read(HL.pair, temp8, [](CPU* c) { c->bit(c->temp8); }); break; // BIT b, (HL)
            case 0x07: bit(AF.hi); break; // BIT b, A
            // RES
            case 0x08: res(BC.hi); break; // RES b, B
            case 0x09: res(BC.lo); break; // RES b, C
            case 0x0A: res(DE.hi); break; // RES b, D
            case 0x0B: res(DE.lo); break; // RES b, E
            case 0x0C: res(HL.hi); break; // RES b, H
            case 0x0D: res(HL.lo); break; // RES b, L
            case 0x0E:
                skd_read(HL.pair, temp8, [](CPU* c) { c->res(c->temp8); });
                skd_write(HL.pair, temp8);
                break; // RES b, (HL)
            case 0x0F: res(AF.hi); break; // RES b, A
            // SET
            case 0x10: set(BC.hi); break; // SET b, B
            case 0x11: set(BC.lo); break; // SET b, C
            case 0x12: set(DE.hi); break; // SET b, D
            case 0x13: set(DE.lo); break; // SET b, E
            case 0x14: set(HL.hi); break; // SET b, H
            case 0x15: set(HL.lo); break; // SET b, L
            case 0x16:
                skd_read(HL.pair, temp8, [](CPU* c) { c->set(c->temp8); });
                skd_write(HL.pair, temp8);
                break; // SET b, (HL)
            case 0x17: set(AF.hi); break; // SET b, A
            default:
                fatal("Impossible bit operation for cb opcode=%02x\n", cbOP);
        }
    }
}
// clang-format on

void CPU::set_flag(u8 flag, bool set) { AF.lo = (AF.lo & ~flag) | (flag * set); }
bool CPU::check_flag(u8 flag) { return (AF.lo & flag) != 0; }
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
    u8 res = AF.hi + val;
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, (AF.hi & 0xF) + (val & 0xF) > 0xF);
    set_flag(C_FLAG, (0xFF - AF.hi) < val);
    AF.hi = res;
}

void CPU::adc8(u8 val) {
    u8 carry = check_flag(C_FLAG);
    u8 res = AF.hi + val + carry;
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, carry + (AF.hi & 0xF) + (val & 0xF) > 0xF);
    set_flag(C_FLAG, val + carry > 0xFF - AF.hi);
    AF.hi = res;
}

void CPU::sub8(u8 val) {
    u8 res = AF.hi - val;
    set_flag(Z_FLAG, AF.hi == val);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (AF.hi & 0xF) < (val & 0xF));
    set_flag(C_FLAG, AF.hi < val);
    AF.hi = res;
}

void CPU::sbc8(u8 val) {
    u8 carry = check_flag(C_FLAG);
    u8 res = AF.hi - (val + carry);
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (AF.hi & 0xF) < ((val & 0xF) + carry));
    set_flag(C_FLAG, AF.hi < val + carry);
    AF.hi = res;
}

void CPU::and8(u8 val) {
    AF.hi &= val;
    set_flag(Z_FLAG, AF.hi == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, true);
    set_flag(C_FLAG, false);
}

void CPU::xor8(u8 val) {
    AF.hi ^= val;
    set_flag(Z_FLAG, AF.hi == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, false);
}

void CPU::or8(u8 val) {
    AF.hi |= val;
    set_flag(Z_FLAG, AF.hi == 0);
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, false);
    set_flag(C_FLAG, false);
}

void CPU::cp8(u8 val) {
    set_flag(Z_FLAG, AF.hi == val);
    set_flag(N_FLAG, true);
    set_flag(H_FLAG, (AF.hi & 0xF) < (val & 0xF));
    set_flag(C_FLAG, AF.hi < val);
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
    u16 res = HL.pair + val;
    set_flag(N_FLAG, false);
    set_flag(H_FLAG, (0xFFF - (HL.pair & 0xFFF)) < (val & 0xFFF));
    set_flag(C_FLAG, (0xFFFF - HL.pair) < val);
    HL.pair = res;
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
