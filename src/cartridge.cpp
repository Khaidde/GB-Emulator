#include "cartridge.hpp"

#include <fstream>

void Cartridge::load_from_path(const char* romPath) {
    std::ifstream file(romPath, std::ios::binary);
    if (!file.is_open()) return;

    file.seekg(0, std::ios::end);

    size_t romSize = file.tellg();
    file.seekg(0);
    file.read((char*)rom, romSize);
    file.close();

    char title[16];
    int i = 0;
    for (int c = 0x134; c < 0x144; c++) {
        title[i++] = rom[c];
    }
    printf("Title: %s\n", title);

    const char* cartridgeType = nullptr;
    switch (rom[0x0147]) {
        case 0x00:
            cartridgeType = "ROM Only";
            break;
        default:
            printf("Unknown cartridge type: %d\n", rom[0x0147]);
    }
    if (cartridgeType) printf("Cartridge Type: %s\n", cartridgeType);

    const char* romSizeDesc = nullptr;
    switch (rom[0x0148]) {
        case 0x00:
            romSizeDesc = "32KByte (no ROM banking)";
            break;
        default:
            printf("Unknown rom size: %d\n", rom[0x0148]);
    }
    if (romSizeDesc) printf("ROM Size: %s\n", romSizeDesc);

    const char* ramSizeDesc = nullptr;
    switch (rom[0x0149]) {
        case 0x00:
            ramSizeDesc = "None";
            break;
        default:
            printf("Unknown ram size: %d\n", rom[0x0149]);
    }
    if (ramSizeDesc) printf("RAM Size: %s\n", ramSizeDesc);

    printf("Destination Code: %s\n", rom[0x014A] ? "Non-Japanese" : "Japanese");
    printf("Version: %d\n", rom[0x14C]);

    int x = 0;
    for (int i = 0x0134; i <= 0x014C; i++) {
        x = x - rom[i] - 1;
    }
    printf("Checksum: %s\n", (x & 0xF) == rom[0x014D] ? "PASSED" : "FAILED");
}