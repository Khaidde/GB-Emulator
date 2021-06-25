#include "game_boy.hpp"

#include <fstream>
#include <vector>

GameBoy::GameBoy() : input(memory), timer(memory), ppu(memory), apu(memory) {
    cpu.set_memory(memory);
    memory.set_input(input);
    memory.set_timer(timer);
    memory.set_ppu(ppu);
    memory.set_apu(apu);
}

namespace {
constexpr int NUM_CART_TYPES = 7;
// clang-format off
constexpr struct {
    const u8 code;
    char mbc;
    bool hasRam;
    bool hasBattery;
} CART_TYPES[NUM_CART_TYPES] = {
    {0x00, 0, false, false},
    {0x01, 1, false, false},
    {0x02, 1, true, false},
    {0x03, 1, true, true},
    {0x19, 5, false, false},
    {0x1A, 5, true, false},
    {0x1B, 5, true, true},
};
//clang-format on
}  // namespace

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

    std::streamsize romSize = file.tellg();
    std::vector<u8> rom((size_t)romSize);
    file.seekg(0);
    file.read((char*)&rom[0], romSize);
    file.close();

    int i = 0;
    for (int i = 0; i < NUM_CART_TYPES; i++) {
        if (rom[0x0147] == CART_TYPES[i].code) {
            CartridgeInfo info;
            std::string romPathStr(romPath);
            info.isCGB = romPathStr.substr(romPathStr.find_last_of(".")) == ".gbc";
            info.savePath = romPathStr.substr(0, romPathStr.find_last_of(".")) + ".sav";
            info.hasRam = CART_TYPES[i].hasRam;
            info.hasBattery = CART_TYPES[i].hasBattery;
            switch (CART_TYPES[i].mbc) {
                case 0:
                    cartridge = std::make_unique<ROMOnly>(info, &rom[0]);
                    break;
                case 1:
                    cartridge = std::make_unique<MBC1>(info, &rom[0]);
                    break;
                case 5:
                    cartridge = std::make_unique<MBC5>(info, &rom[0]);
                    break;
            }
            if (CART_TYPES[i].hasBattery) {
                cartridge->try_load_save_file();
            }
        }
    }
    if (i == NUM_CART_TYPES) {
        fatal("Unimplemented cartridge type: %d\n", rom[0x0147]);
    }
    memory.set_cartridge(cartridge.get());
}

void GameBoy::set_debugger(Debugger& debugger) {
    this->debugger = &debugger;
    debugger.init(&cpu, &ppu, &memory);

    cpu.set_debugger(debugger);
    memory.set_debugger(debugger);
    ppu.set_debugger(debugger);
}

void GameBoy::emulate_frame() {
    while (totalTCycles < PPU::TOTAL_CLOCKS) {
        if (cpu.is_fetching() && debugger->is_paused() && !debugger->can_step()) {
            break;
        }
        if (cpu.is_fetching() && debugger->is_paused()) {
            printf("---------------\n");
        }

        memory.emulate_dma_cycle();
        if (cpu.is_fetching()) {
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
