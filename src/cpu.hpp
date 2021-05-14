#pragma once

#include "debugger.hpp"
#include "memory.hpp"
#include "utils.hpp"

class Memory;
class Debugger;

union RegPair {
    struct {
        u8 lo;
        u8 hi;
    };
    u16 pair;
};

class CPU {
   public:
    void init(Memory* memory, Debugger* debugger);
    void restart();

    void handle_interrupts();
    bool isFetching();
    void emulate_cycle();

   private:
    friend class Debugger;
    Debugger* debugger;

    Memory* memory;

    RegPair AF;
    RegPair BC;
    RegPair DE;
    RegPair HL;

    u16 SP;
    u16 PC;

    u8 valReg;  // temporary register used for delayed memory read and writes

    bool halted;
    bool haltBug;
    bool ime;
    bool imeScheduled;

    static constexpr u8 Z_FLAG = 1 << 7;  // Zero flag
    static constexpr u8 N_FLAG = 1 << 6;  // Subtract flag
    static constexpr u8 H_FLAG = 1 << 5;  // Half Carry flag
    static constexpr u8 C_FLAG = 1 << 4;  // Carry flag

    u8 cycleCnt;
    void set_flag(u8 flag, bool set);
    bool check_flag(u8 flag);
    u8 n();
    u16 nn();
    u8 read(u16 addr);
    void write(u16 addr, u8 val);

    u8 readCycle;
    using ReadCallback = void (*)(CPU* c);
    ReadCallback readCallback;
    void skd_read(u8& dest, u16 addr, ReadCallback&& callback);
    void skd_read(u8& dest, u16 addr);
    void skd_write(u16 addr, u8& val);

    // ----- Instructions -----
    void op00();  // NOP
    void op10();  // STOP
    void op76();  // HALT

    // ----- Interrupts -----
    void opF3();  // DI
    void opFB();  // EI

    // ----- 8-bit -----

    // LD (16), A
    void op02();  // LD (BC), A
    void op12();  // LD (DE), A
    void op22();  // LD (HL+), A
    void op32();  // LD (HL-), A

    // 8-bit load
    void op06();  // LD B, n
    void op16();  // LD D, n
    void op26();  // LD H, n
    void op36();  // LD (HL), n
    void op0E();  // LD C, n
    void op1E();  // LD E, n
    void op2E();  // LD L, n
    void op3E();  // LD A, n

    // LD A, (16)
    void op0A();  // LD A, (BC)
    void op1A();  // LD A, (DE)
    void op2A();  // LD A, (HL+)
    void op3A();  // LD A, (HL-)

    // LD B, r2
    void op40();  // LD B, B
    void op41();  // LD B, C
    void op42();  // LD B, D
    void op43();  // LD B, E
    void op44();  // LD B, H
    void op45();  // LD B, L
    void op46();  // LD B, (HL)
    void op47();  // LD B, A

    // LD C, r2
    void op48();  // LD C, B
    void op49();  // LD C, C
    void op4A();  // LD C, D
    void op4B();  // LD C, E
    void op4C();  // LD C, H
    void op4D();  // LD C, L
    void op4E();  // LD C, (HL)
    void op4F();  // LD C, A

    // LD D, r2
    void op50();  // LD D, B
    void op51();  // LD D, C
    void op52();  // LD D, D
    void op53();  // LD D, E
    void op54();  // LD D, H
    void op55();  // LD D, L
    void op56();  // LD D, (HL)
    void op57();  // LD D, A

    // LD E, r2
    void op58();  // LD E, B
    void op59();  // LD E, C
    void op5A();  // LD E, D
    void op5B();  // LD E, E
    void op5C();  // LD E, H
    void op5D();  // LD E, L
    void op5E();  // LD E, (HL)
    void op5F();  // LD E, A

    // LD H, r2
    void op60();  // LD H, B
    void op61();  // LD H, C
    void op62();  // LD H, D
    void op63();  // LD H, E
    void op64();  // LD H, H
    void op65();  // LD H, L
    void op66();  // LD H, (HL)
    void op67();  // LD H, A

    // LD L, r2
    void op68();  // LD L, B
    void op69();  // LD L, C
    void op6A();  // LD L, D
    void op6B();  // LD L, E
    void op6C();  // LD L, H
    void op6D();  // LD L, L
    void op6E();  // LD L, (HL)
    void op6F();  // LD L, A

    // LD (HL), r2
    void op70();  // LD (HL), B
    void op71();  // LD (HL), C
    void op72();  // LD (HL), D
    void op73();  // LD (HL), E
    void op74();  // LD (HL), H
    void op75();  // LD (HL), L
    void op77();  // LD (HL), A

    // LD A, r2
    void op78();  // LD A, B
    void op79();  // LD A, C
    void op7A();  // LD A, D
    void op7B();  // LD A, E
    void op7C();  // LD A, H
    void op7D();  // LD A, L
    void op7E();  // LD A, (HL)
    void op7F();  // LD A, A

    // load $FF00 + n
    void opE0();  // LDH ($FF00 + n), A
    void opF0();  // LDH A, ($FF00 + n)

    // load $FF00 + C
    void opE2();  // LD (C), A [A -> $FF00 + C]
    void opF2();  // LD A, (C) [$FF00 + C -> A]

    void opEA();  // LD nn, A
    void opFA();  // LD A, nn

    // ----- 16-bit -----

    // LD n, nn
    void op01();  // LD BC, nn
    void op11();  // LD DE, nn
    void op21();  // LD HL, nn
    void op31();  // LD SP, nn

    // pop
    u16 pop();
    void opC1();  // POP BC
    void opD1();  // POP DE
    void opE1();  // POP HL
    void opF1();  // POP AF

    // push
    void push(u16 val);
    void opC5();  // PUSH BC
    void opD5();  // PUSH DE
    void opE5();  // PUSH HL
    void opF5();  // PUSH AF

    void op08();  // LD nn, SP

    void opF9();  // LD SP, HL

    // ----- 8-bit ALU -----

    // ADD A, n
    void add8_to_A(u8 val);
    void op80();  // ADD A, B
    void op81();  // ADD A, C
    void op82();  // ADD A, D
    void op83();  // ADD A, E
    void op84();  // ADD A, H
    void op85();  // ADD A, L
    void op86();  // ADD A, (HL)
    void op87();  // ADD A, A
    void opC6();  // ADD A, n

    // ADC A, n
    void adc8_to_A(u8 val);
    void op88();  // ADC A, B
    void op89();  // ADC A, C
    void op8A();  // ADC A, D
    void op8B();  // ADC A, E
    void op8C();  // ADC A, H
    void op8D();  // ADC A, L
    void op8E();  // ADC A, (HL)
    void op8F();  // ADC A, A
    void opCE();  // ADC A, n

    // SUB A, n
    void sub8_to_A(u8 val);
    void op90();  // SUB B
    void op91();  // SUB C
    void op92();  // SUB D
    void op93();  // SUB E
    void op94();  // SUB H
    void op95();  // SUB L
    void op96();  // SUB (HL)
    void op97();  // SUB A
    void opD6();  // SUB n

    // SBC A, n
    void sbc8_to_A(u8 val);
    void op98();  // SBC A, B
    void op99();  // SBC A, C
    void op9A();  // SBC A, D
    void op9B();  // SBC A, E
    void op9C();  // SBC A, H
    void op9D();  // SBC A, L
    void op9E();  // SBC A, (HL)
    void op9F();  // SBC A, A
    void opDE();  // SBC A, n

    // AND A, n
    void and8_to_A(u8 val);
    void opA0();  // AND B
    void opA1();  // AND C
    void opA2();  // AND D
    void opA3();  // AND E
    void opA4();  // AND H
    void opA5();  // AND L
    void opA6();  // AND (HL)
    void opA7();  // AND A
    void opE6();  // AND n

    // XOR A, n
    void xor8_to_A(u8 val);
    void opA8();  // XOR B
    void opA9();  // XOR C
    void opAA();  // XOR D
    void opAB();  // XOR E
    void opAC();  // XOR H
    void opAD();  // XOR L
    void opAE();  // XOR (HL)
    void opAF();  // XOR A
    void opEE();  // XOR A, n

    // OR A, n
    void or8_to_A(u8 val);
    void opB0();  // OR B
    void opB1();  // OR C
    void opB2();  // OR D
    void opB3();  // OR E
    void opB4();  // OR H
    void opB5();  // OR L
    void opB6();  // OR (HL)
    void opB7();  // OR A
    void opF6();  // OR n

    // CP A, n
    void compare8(u8 val);
    void opB8();  // CP B
    void opB9();  // CP C
    void opBA();  // CP D
    void opBB();  // CP E
    void opBC();  // CP H
    void opBD();  // CP L
    void opBE();  // CP (HL)
    void opBF();  // CP A
    void opFE();  // CP n

    // INC n
    void inc8(u8& reg);
    void op04();  // INC B
    void op0C();  // INC C
    void op14();  // INC D
    void op1C();  // INC E
    void op24();  // INC H
    void op2C();  // INC L
    void op34();  // INC (HL)
    void op3C();  // INC A

    // DEC n
    void dec8(u8& reg);
    void op05();  // DEC B
    void op0D();  // DEC C
    void op15();  // DEC D
    void op1D();  // DEC E
    void op25();  // DEC H
    void op2D();  // DEC L
    void op35();  // DEC (HL)
    void op3D();  // DEC A

    // ----- 16-bit ALU -----

    // ADD HL, n
    void add16_to_HL(u16 val);
    void op09();  // ADD HL, BC
    void op19();  // ADD HL, DE
    void op29();  // ADD HL, HL
    void op39();  // ADD HL, SP

    u16 addSP_n();
    void opE8();  // ADD SP, n
    void opF8();  // LD HL, SP + n

    // INC nn
    void op03();  // INC BC
    void op13();  // INC DE
    void op23();  // INC HL
    void op33();  // INC SP

    void op0B();  // DEC BC
    void op1B();  // DEC DE
    void op2B();  // DEC HL
    void op3B();  // DEC SP

    // ----- Misc ALU -----

    // Rotate Left
    void op07();  // RLCA
    void op17();  // RLA

    // Rotate right
    void op0F();  // RRCA
    void op1F();  // RRA

    void op27();  // DAA
    void op37();  // SCF

    void op2F();  // CPL
    void op3F();  // CCF

    // ----- Jump -----
    void jump(u16 addr);

    void opC3();  // JP nn

    void opC2();  // JP NZ, nn
    void opD2();  // JP NC, nn
    void opCA();  // JP Z, nn
    void opDA();  // JP C, nn

    void opE9();  // JP HL

    void op18();  // JR n
    void op20();  // JR NZ, n
    void op30();  // JR NC, n
    void op28();  // JR Z, n
    void op38();  // JR C, n

    // ----- Call -----
    void call(u16 addr);

    void opCD();  // CALL

    void opC4();  // CALL NZ, nn
    void opD4();  // CALL NC, nn

    void opCC();  // CALL Z, nn
    void opDC();  // CALL C, nn

    // ----- Restarts -----
    void opC7();  // RST 00H
    void opD7();  // RST 10H
    void opE7();  // RST 20H
    void opF7();  // RST 30H

    void opCF();  // RST 08H
    void opDF();  // RST 18H
    void opEF();  // RST 28H
    void opFF();  // RST 38H

    // ----- Returns -----
    void opC9();  // RET

    void opC0();  // RET NZ
    void opD0();  // RET NC
    void opC8();  // RET Z
    void opD8();  // RET C

    void opD9();  // RETI

    // ----- Error Opcodes -----
    void freeze();
    void opD3();
    void opE3();
    void opE4();
    void opF4();
    void opDB();
    void opEB();
    void opEC();
    void opFC();
    void opDD();
    void opED();
    void opFD();

    // ----- CB-prefix -----
    void opCB();  // CB-prefix

    // SWAP n
    void swap(u8& reg);
    void opCB30();  // SWAP B
    void opCB31();  // SWAP C
    void opCB32();  // SWAP D
    void opCB33();  // SWAP E
    void opCB34();  // SWAP H
    void opCB35();  // SWAP L
    void opCB36();  // SWAP (HL)
    void opCB37();  // SWAP A

    //  RLC n
    void rlc8(u8& reg);
    void opCB00();  // RLC B
    void opCB01();  // RLC C
    void opCB02();  // RLC D
    void opCB03();  // RLC E
    void opCB04();  // RLC H
    void opCB05();  // RLC L
    void opCB06();  // RLC (HL)
    void opCB07();  // RLC A

    //  RRC n
    void rrc8(u8& reg);
    void opCB08();  // RRC B
    void opCB09();  // RRC C
    void opCB0A();  // RRC D
    void opCB0B();  // RRC E
    void opCB0C();  // RRC H
    void opCB0D();  // RRC L
    void opCB0E();  // RRC (HL)
    void opCB0F();  // RRC A

    // RL n
    void rl8(u8& reg);
    void opCB10();  // RL B
    void opCB11();  // RL C
    void opCB12();  // RL D
    void opCB13();  // RL E
    void opCB14();  // RL H
    void opCB15();  // RL L
    void opCB16();  // RL (HL)
    void opCB17();  // RL A

    // RR n
    void rr8(u8& reg);
    void opCB18();  // RR B
    void opCB19();  // RR C
    void opCB1A();  // RR D
    void opCB1B();  // RR E
    void opCB1C();  // RR H
    void opCB1D();  // RR L
    void opCB1E();  // RR (HL)
    void opCB1F();  // RR A

    // SLA n
    void sla8(u8& reg);
    void opCB20();  // SLA B
    void opCB21();  // SLA C
    void opCB22();  // SLA D
    void opCB23();  // SLA E
    void opCB24();  // SLA H
    void opCB25();  // SLA L
    void opCB26();  // SLA (HL)
    void opCB27();  // SLA A

    // SRA n
    void sra8(u8& reg);
    void opCB28();  // SRA B
    void opCB29();  // SRA C
    void opCB2A();  // SRA D
    void opCB2B();  // SRA E
    void opCB2C();  // SRA H
    void opCB2D();  // SRA L
    void opCB2E();  // SRA (HL)
    void opCB2F();  // SRA A

    // SRL n
    void srl8(u8& reg);
    void opCB38();  // SRL B
    void opCB39();  // SRL C
    void opCB3A();  // SRL D
    void opCB3B();  // SRL E
    void opCB3C();  // SRL H
    void opCB3D();  // SRL L
    void opCB3E();  // SRL (HL)
    void opCB3F();  // SRL A

    // BIT b, r
    void bit(u8 b, u8 reg);
    void opCB_bitB(u8 b);   // BIT b, B
    void opCB_bitC(u8 b);   // BIT b, C
    void opCB_bitD(u8 b);   // BIT b, D
    void opCB_bitE(u8 b);   // BIT b, E
    void opCB_bitH(u8 b);   // BIT b, H
    void opCB_bitL(u8 b);   // BIT b, L
    void opCB_bitHL(u8 b);  // BIT b, (HL)
    void opCB_bitA(u8 b);   // BIT b, A

    // RES b, r
    void res(u8 b, u8& reg);
    void opCB_resB(u8 b);   // RES b, B
    void opCB_resC(u8 b);   // RES b, C
    void opCB_resD(u8 b);   // RES b, D
    void opCB_resE(u8 b);   // RES b, E
    void opCB_resH(u8 b);   // RES b, H
    void opCB_resL(u8 b);   // RES b, L
    void opCB_resHL(u8 b);  // RES b, (HL)
    void opCB_resA(u8 b);   // RES b, A

    // SET b, r
    void set(u8 b, u8& reg);
    void opCB_setB(u8 b);   // SET b, B
    void opCB_setC(u8 b);   // SET b, C
    void opCB_setD(u8 b);   // SET b, D
    void opCB_setE(u8 b);   // SET b, E
    void opCB_setH(u8 b);   // SET b, H
    void opCB_setL(u8 b);   // SET b, L
    void opCB_setHL(u8 b);  // SET b, (HL)
    void opCB_setA(u8 b);   // SET b, A

    using OpcodeFuncPtr = void (CPU::*)();
    OpcodeFuncPtr opcodes[256];

    OpcodeFuncPtr opcodesCB[256];

    using BitOpcodeFuncPtr = void (CPU::*)(u8);
    BitOpcodeFuncPtr bitOpcodesCB[24] = {
        opCB_bitB, opCB_bitC, opCB_bitD, opCB_bitE, opCB_bitH, opCB_bitL, opCB_bitHL, opCB_bitA,
        opCB_resB, opCB_resC, opCB_resD, opCB_resE, opCB_resH, opCB_resL, opCB_resHL, opCB_resA,
        opCB_setB, opCB_setC, opCB_setD, opCB_setE, opCB_setH, opCB_setL, opCB_setHL, opCB_setA,
    };
};