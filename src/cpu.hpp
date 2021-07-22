#pragma once

#include "debugger.hpp"
#include "general.hpp"
#include "memory.hpp"

class Memory;
class Debugger;

class CPU {
public:
    CPU(Memory& memory);
    void set_debugger(Debugger& debug) { this->debugger = &debug; }
    void restart();

    void handle_interrupts();
    void fetch_execute();

private:
    friend class Debugger;
    Debugger* debugger;

    Memory* memory;

    // clang-format off
    union {
        struct {
            u16 AF;
            u16 BC;
            u16 DE;
            u16 HL;
        };
        // TODO only works on little-endian machines
        struct {
            u8 F; u8 A;
            u8 C; u8 B;
            u8 E; u8 D;
            u8 L; u8 H;
        };
    } regs;
    // clang-format on

    u16 SP;
    u16 PC;

    bool halted;
    bool haltBug;
    bool ime;
    bool imeScheduled;

    void execute(u8 opcode);
    void execute_cb();

    enum class Flag : u8 {
        Z = 1 << 7,  // Zero flag
        N = 1 << 6,  // Subtract flag
        H = 1 << 5,  // Half Carry flag
        C = 1 << 4,  // Carry flag
    };
    void set_flag(Flag flag, bool set);
    bool check_flag(Flag flag);

    u8 n();
    u16 nn();
    u8 read(u16 addr);
    void write(u16 addr, u8 val);

    void pop(u16& dest);
    void push(u16 val);

    void add8(u8 val);
    void adc8(u8 val);
    void sub8(u8 val);
    void sbc8(u8 val);

    void and8(u8 val);
    void xor8(u8 val);
    void or8(u8 val);
    void cp8(u8 val);
    void inc8(u8& reg);
    void dec8(u8& reg);

    void add16(u16 val);

    u16 addSP_n();

    void daa();

    void jump_nn();
    void call_nn();
    void rst(u16 addr);

    void freeze();

    void rlc8(u8& reg);
    void rrc8(u8& reg);

    void rl8(u8& reg);
    void rr8(u8& reg);

    void sla8(u8& reg);
    void sra8(u8& reg);

    void swap(u8& reg);
    void srl8(u8& reg);

    void bit(u8 reg, u8 bit);
    void res(u8& reg, u8 bit);
    void set(u8& reg, u8 bit);
};
