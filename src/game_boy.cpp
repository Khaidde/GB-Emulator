#include "game_boy.hpp"

GameBoy::GameBoy() : input(&memory), timer(&memory), ppu(&memory), apu(&memory) {
    cpu.set_memory(memory);
    memory.set_input(input);
    memory.set_timer(timer);
    memory.set_ppu(ppu);
    memory.set_apu(apu);
}

void GameBoy::load(const char* romPath) {
    cpu.restart();
    memory.restart();
    input.restart();
    timer.restart();
    ppu.restart();
    apu.restart();

    memory.load_cartridge(romPath);
}

void GameBoy::emulate_frame(u32* screenBuffer, s16* sampleBuffer, u16 sampleLen) {
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
    ppu.render(screenBuffer);
    apu.sample(sampleBuffer, sampleLen);
    totalTCycles -= PPU::TOTAL_CLOCKS;
}