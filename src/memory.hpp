#pragma once

#include <memory>

#include "apu.hpp"
#include "cartridge.hpp"
#include "debugger.hpp"
#include "general.hpp"
#include "input.hpp"
#include "ppu.hpp"
#include "timer.hpp"

class Debugger;
class Timer;
class PPU;
class APU;

class Memory {
public:
    void restart();
    void set_cartridge(Cartridge* cartridge) { this->cartridge = cartridge; }
    void set_debugger(Debugger& debugger) { this->debugger = &debugger; }
    void set_input(Input& in) { this->input = &in; }
    void set_timer(Timer& time) { this->timer = &time; }
    void set_apu(APU& a) { this->apu = &a; }
    void set_ppu(PPU& p) { this->ppu = &p; }

    bool is_CGB() { return cartridge->is_CGB(); }
    bool is_CGB_mode() { return cartridge->is_CGB_mode(); }
    bool is_DMG_mode() { return cartridge->is_DMG_mode(); }

    void request_interrupt(Interrupt interrupt);

    u8& ref(u16 addr);
    u8 read(u16 addr);
    void write(u16 addr, u8 val);

    bool is_double_speed() { return isDoubleSpeed; }
    bool should_speed_switch() { return prepareSpeedSwitch; }
    void start_speed_switch();
    bool is_speed_switching();

    bool is_hdma_ongoing();
    void continue_hblank_dma();

    int get_elapsed_cycles();
    void reset_elapsed_cycles();
    void sleep_cycle();

private:
    void emulate_speed_switch_cycle();
    void emulate_dma_cycle();
    void emulate_hdma_2clock();

private:
    friend class Debugger;
    Debugger* debugger;

    Cartridge* cartridge;

    Input* input;
    Timer* timer;
    APU* apu;
    PPU* ppu;

    static constexpr int MEMORY_SIZE = 0x10000;
    u8 mem[MEMORY_SIZE];

    using WRAMBank = u8[0x1000];
    WRAMBank* curWramBank;
    WRAMBank wramBanks[8];  // WRAM banks 1-7

    bool isDoubleSpeed;
    bool prepareSpeedSwitch;
    int speedSwitchCycleCnt;

    bool dmaInProgress;
    bool scheduleDma;
    u16 dmaStartAddr;
    u8 dmaCycleCnt;

    u16 hdmaSource;
    u16 hdmaDest;
    u16 hdmaBytesLeft;
    u16 hdmaBytesTransferred;
    u16 hdmaLen;
    u16 hdma2ClockCnt;

    int elapsedCycles;
};
