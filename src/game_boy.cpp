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
constexpr int NUM_CART_TYPES = 4;
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
};
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

    /*
    isCGB = FileManagement::is_path_extension(romPath, ".gbc");
    if (isCGB) {
        fatal("TODO gameboy color games are not yet supported!\n");
    }
    */

    std::streamsize romSize = file.tellg();
    std::vector<u8> rom((size_t)romSize);
    file.seekg(0);
    file.read((char*)&rom[0], romSize);
    file.close();

    int i = 0;
    for (int i = 0; i < NUM_CART_TYPES; i++) {
        if (rom[0x0147] == CART_TYPES[i].code) {
            switch (CART_TYPES[i].mbc) {
                case 0:
                    cartridge = std::make_unique<ROMOnly>(romPath, &rom[0]);
                    break;
                case 1:
                    cartridge = std::make_unique<MBC1>(romPath, &rom[0]);
            }
            cartridge->set_has_ram(CART_TYPES[i].hasRam);
            cartridge->set_has_battery(CART_TYPES[i].hasBattery);
            if (CART_TYPES[i].hasBattery) {
                std::string savePath = FileManagement::change_extension(romPath, ".sav");
                std::ifstream saveFile(savePath, std::ios::binary | std::ios::ate);
                if (saveFile.is_open()) {
                    printf("Loading save file: %s\n", savePath.c_str());
                    std::streamsize ramSize = saveFile.tellg();
                    std::vector<u8> ram((size_t)ramSize);
                    saveFile.seekg(0);
                    saveFile.read((char*)&ram[0], ramSize);
                    saveFile.close();
                    cartridge->load_save_ram(&ram[0]);
                }
            }
        }
    }
    if (i == NUM_CART_TYPES) {
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

u8 GameBoy::get_serial_out() { return memory.read(IOReg::SB_REG); }
