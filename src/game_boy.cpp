#include "game_boy.hpp"

GameBoy::GameBoy() : input(&memory), timer(&memory), ppu(&memory), apu(&memory) {
    cpu.set_memory(memory);
    memory.set_input(input);
    memory.set_timer(timer);
    memory.set_ppu(ppu);
    memory.set_apu(apu);
}

void GameBoy::load(const char* romPath) {
    memory.load_cartridge(romPath);

    cpu.restart();
    memory.restart();
    input.restart();
    timer.restart();
    ppu.restart();
    apu.restart();
}

void GameBoy::print_cartridge_info() { memory.print_cartridge_info(); }

void GameBoy::set_debugger(Debugger& debugger) {
    this->debugger = &debugger;
    debugger.init(&cpu, &memory);

    cpu.set_debugger(debugger);
    memory.set_debugger(debugger);
    ppu.set_debugger(debugger);
}

void GameBoy::emulate_frame() {
    while (totalTCycles < PPU::TOTAL_CLOCKS) {
        if (cpu.isFetching() && debugger->is_paused() && !debugger->can_step()) {
            break;
        }
        if (cpu.isFetching() && debugger->is_paused()) {
            printf("---------------\n");
        }

        memory.emulate_dma_cycle();
        if (cpu.isFetching()) {
            memory.reset_cycles();
            cpu.handle_interrupts();  // Interrupts are checked before fetching a new instruction
        }
        for (int i = 0; i < 4; i++) {
            timer.emulate_clock();
            ppu.emulate_clock();
            apu.emulate_clock();
        }
        cpu.emulate_cycle();
        memory.emulate_cycle();
        totalTCycles += 4;
    }
    totalTCycles -= PPU::TOTAL_CLOCKS;
}

void GameBoy::emulate_frame(u32* screenBuffer, s16* sampleBuffer, u16 sampleLen) {
    emulate_frame();
    ppu.render(screenBuffer);
    if (debugger->is_paused()) {
        for (int i = 0; i < sampleLen; i++) {
            sampleBuffer[i] = 0;
        }
    } else {
        apu.sample(sampleBuffer, sampleLen);
    }
}

char GameBoy::get_serial_out() { return memory.read(IOReg::SB_REG); }