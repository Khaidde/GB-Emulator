#include "game_boy.hpp"

#include <fstream>
#include <vector>

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

    std::ifstream file(romPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        fatal("Can't read from file: %s\n", romPath);
    }

    /*
    isCGB = FileManagement::is_path_extension(romPath, ".gbc");
    if (isCGB) {
        fatal("TODO gameboy color games are not yet supported!\n");
    }
    */

    size_t romSize = file.tellg();
    std::vector<u8> rom(romSize);
    file.seekg(0);
    file.read((char*)&rom[0], romSize);
    file.close();

    switch (rom[0x0147]) {
        case 0x00:
            cartridge = std::make_unique<ROMOnly>(&rom[0]);
            break;
        case 0x01:
            cartridge = std::make_unique<MBC1>(&rom[0]);
            break;
        case 0x02:
            cartridge = std::make_unique<MBC1>(&rom[0]);
            cartridge->setHasRam(true);
            break;
        case 0x03:
            cartridge = std::make_unique<MBC1>(&rom[0]);
            cartridge->setHasRam(true);
            cartridge->setHasBattery(true);
            break;
        default:
            fatal("Unimplemented cartridge type: %d\n", rom[0x0147]);
    }
    memory.set_cartridge(cartridge.get());
}

Cartridge* GameBoy::get_cartridge() { return cartridge.get(); }

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