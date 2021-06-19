#pragma once

#include "debugger.hpp"
#include "memory.hpp"
#include "general.hpp"

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
    CPU() { restart(); }
    void set_memory(Memory& memory) { this->memory = &memory; }
    void set_debugger(Debugger& debugger) { this->debugger = &debugger; }
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

    u8 temp8;  // temporary register used for delayed memory read and writes

    u8 bitReg;  // bit select for cb operations above 0x40

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

    using Callback = void (*)(CPU* c);
    u8 callbackCycle;
    Callback callback;

    void skd_read(u16 addr, u8& dest, Callback&& readCallback);
    void skd_read(u16 addr, u8& dest);
    void skd_write(u16 addr, u8& val);

    void execute(u8 opcode);
    void execute_cb();

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

    void bit(u8 reg);
    void res(u8& reg);
    void set(u8& reg);
};
