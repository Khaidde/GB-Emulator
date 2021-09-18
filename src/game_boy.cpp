#include "game_boy.hpp"

#include <fstream>
#include <vector>

GameBoy::GameBoy()
    : cpu(memory), input(memory), timer(memory), serial(memory), apu(memory), ppu(memory) {
    memory.set_input(input);
    memory.set_timer(timer);
    memory.set_serial(serial);
    memory.set_apu(apu);
    memory.set_ppu(ppu);
}

namespace {
constexpr int NUM_CART_TYPES = 14;
// clang-format off
constexpr struct {
    const u8 code;
    char mbc;
    bool hasRam;
    bool hasBattery;
    bool hasTimer;
} CART_TYPES[NUM_CART_TYPES] = {
    {0x00, 0, false, false, false},

    {0x01, 1, false, false, false},
    {0x02, 1, true, false, false},
    {0x03, 1, true, true, false},

    {0x05, 2, false, false, false},
    {0x06, 2, false, true, false},

    {0x0F, 3, false, true, true},
    {0x10, 3, true, true, true},
    {0x11, 3, false, false, false},
    {0x12, 3, true, false, false},
    {0x13, 3, true, true, false},

    {0x19, 5, false, false, false},
    {0x1A, 5, true, false, false},
    {0x1B, 5, true, true, false},
};
//clang-format on
}  // namespace

void GameBoy::load(const char* romPath) {
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
                case 2:
                    cartridge = std::make_unique<MBC2>(info, &rom[0]);
                    break;
                case 3:
                    cartridge = std::make_unique<MBC3>(info, &rom[0]);
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

    cpu.restart();
    memory.restart();
    input.restart();
    timer.restart();
    serial.restart();
    ppu.restart();
    apu.restart();
}

void GameBoy::set_debugger(Debugger& debugger) {
    this->debugger = &debugger;
    debugger.init(&cpu, &memory);

    cpu.set_debugger(debugger);
    memory.set_debugger(debugger);
    ppu.set_debugger(debugger);
    timer.set_debugger(debugger);
    serial.set_debugger(debugger);
}

void GameBoy::emulate_frame() {
    bool frameElapsed = true;
    while (memory.get_elapsed_cycles() < PPU::TOTAL_CLOCKS << memory.is_double_speed()) {
        if (debugger->is_paused() && !debugger->can_step() && !memory.is_hdma_ongoing()) {
            frameElapsed = false;
            break;
        }
        cpu.fetch_execute();
    }
    if (frameElapsed) {
        memory.reset_elapsed_cycles();
    }
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
