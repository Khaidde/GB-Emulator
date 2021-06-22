#include "cartridge.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace {
// clang-format off
constexpr const char* ROM_SIZES[] = {
    "32 KByte (no ROM banking)",
    "64 KByte (4 banks)",
    "128 KByte (8 banks)",
    "256 KByte (16 banks)",
    "512 KByte (32 banks)",
    "1 MByte (64 banks)",
    "2 MByte (128 banks)",
    "4 MByte (256 banks)",
    "8 MByte (512 banks)",
};
constexpr struct {
    const char* desc;
    u16 numBanks;
} RAM_SIZES[] = {
    {"None", 0},
    {"2 KBytes", 1},
    {"8 KBytes", 1},
    {"32 KBytes (4 banks of 8KBytes)", 4},
    {"128 KBytes (16 banks of 8KBytes)", 16},
    {"64 KBytes (8 banks of 8KBytes)", 8},
};
// clang-format on
}  // namespace

Cartridge::Cartridge(const char* cartName, size_t maxRomBanks, size_t maxRamBanks,
                     const char* filePath, u8* rom)
    : cartridgeName(cartName),
      MAX_ROM_BANKS(maxRomBanks),
      MAX_RAM_BANKS(maxRamBanks),
      filePath(filePath) {
    isCGB = FileManagement::is_path_extension(filePath, ".gbc");
    if (isCGB) {
        fatal("TODO gameboy color games are not yet supported!\n");
    }

    for (int i = 0; i < HEADER_SIZE; i++) {
        header[i] = rom[HEADER_START + i];
    }

    romType = rom[0x148];
    numRomBanks = 2;
    if (romType <= 0x8) {
        for (int i = 0; i < romType; i++) {
            numRomBanks *= 2;
        }
    } else if (0x52 <= romType && romType <= 0x54) {
        fatal("Unsupported rom size: %d\n", romType);
    } else {
        fatal("Unknown rom size: %d\n", romType);
    }
    if (numRomBanks > MAX_ROM_BANKS) {
        fatal("Incompatible number of rom banks for %s: %d\n", cartName, numRomBanks);
    }
    this->romBanks = new RomBank[numRomBanks];
    for (int i = 0; i < numRomBanks; i++) {
        for (int k = 0; k < ROM_BANK_SIZE; k++) {
            romBanks[i][k] = rom[i * ROM_BANK_SIZE + k];
        }
    }
    this->zeroBank = &romBanks[0];
    this->highBank = &romBanks[1];

    ramType = rom[0x149];
    if (ramType <= 0x5) {
        if (ramType == 1) {
            fatal("TODO check that 2KByte ram works correctly\n");
        }
        numRamBanks = RAM_SIZES[ramType].numBanks;
    } else {
        fatal("Unknown ram size: %d\n", ramType);
    }
    if (numRamBanks > 0) {
        this->ramBanks = new RamBank[numRamBanks];
        this->activeRamBank = &ramBanks[0];

        this->ramg = false;
    }
}

Cartridge::~Cartridge() {
    if (hasBattery) {
        std::vector<u8> ram;
        ram.reserve(numRamBanks * RAM_BANK_SIZE);
        for (int i = 0; i < numRamBanks; i++) {
            ram.insert(ram.begin() + (i * RAM_BANK_SIZE), std::begin(ramBanks[i]),
                       std::end(ramBanks[i]));
        }

        std::string savePath = FileManagement::change_extension(filePath, ".sav");
        printf("Saving game to file: %s\n", savePath.c_str());

        std::ofstream saveFileStream(savePath, std::ios::binary);
        saveFileStream.write((char*)ram.data(), (std::streamsize)ram.size());
        saveFileStream.close();
    }
    delete[] romBanks;
    if (hasRam) {
        delete[] ramBanks;
    }
}

void Cartridge::load_save_file(const char* savePath) {
    std::ifstream saveFile(savePath, std::ios::binary | std::ios::ate);
    if (saveFile.is_open()) {
        printf("Loading save file: %s\n", savePath);
        std::streamsize ramSize = saveFile.tellg();

        std::vector<u8> ram;
        ram.reserve((size_t)ramSize);
        saveFile.seekg(0);
        saveFile.read((char*)ram.data(), ramSize);
        saveFile.close();

        for (size_t i = 0; i < numRamBanks; i++) {
            for (size_t k = 0; k < RAM_BANK_SIZE; k++) {
                this->ramBanks[i][k] = ram[i * RAM_BANK_SIZE + k];
            }
        }
    }
}

void Cartridge::print_cartridge_info() {
    u8 title[15];
    int t = 0;
    for (int c = 0x134; c < 0x143; c++) {
        title[t++] = read_header(c);
    }
    printf("Title: %s\n", title);

    // 80h - Game supports CGB functions, but works on old gameboys also.
    // C0h - Game works on CGB only (physically the same as 80h).
    if (read_header(0x143) == 0x80) {
        printf("CGB Flag: also works on old gameboys\n");
    } else if (read_header(0x143) == 0xC0) {
        printf("CGB Flag: only works on CGB\n");
    }

    if (cartridgeName[3] == '1') {
        if (romType == 5) {
            printf("ROM Size: 1 Mbyte (63 banks on MBC1)\n");
        } else if (romType == 6) {
            printf("ROM Size: 2 Mbyte (127 banks on MBC1)\n");
        } else {
            printf("ROM Size: %s\n", ROM_SIZES[romType]);
        }
    } else {
        printf("ROM Size: %s\n", ROM_SIZES[romType]);
    }
    printf("RAM Size: %s\n", RAM_SIZES[ramType].desc);

    if (read_header(0x146) == 0x00) {
        printf("No SGB functions\n");
    } else if (read_header(0x146) == 0x03) {
        printf("Game supports SGB functions\n");
    }

    std::string cartridgeType(cartridgeName);
    if (hasRam) cartridgeType += " + ram";
    if (hasBattery) cartridgeType += " + battery";
    if (hasTimer) cartridgeType += " + timer";
    if (hasRumble) cartridgeType += " + rumble";
    printf("Cartridge Type: %s\n", cartridgeType.c_str());

    printf("Destination Code: %s\n", read_header(0x014A) ? "Non-Japanese" : "Japanese");
    printf("Version: %d\n", read_header(0x14C));

    int x = 0;
    for (int i = 0x0134; i <= 0x014C; i++) {
        x = x - read_header(i) - 1;
    }
    printf("Checksum: %s\n", ((x & 0xF) == read_header(0x014D)) ? "PASSED" : "FAILED");
}

u8 Cartridge::read_header(u16 addr) { return header[addr - HEADER_START]; }

u8 Cartridge::read_rom(u16 addr) {
    if (addr < ROM_BANK_SIZE) {
        return (*zeroBank)[addr];
    }
    if (addr < ROM_BANK_SIZE * 2) {
        return (*highBank)[addr & 0x3FFF];
    }
    fatal("Invalid rom address: %04x\n", addr);
}

u8 Cartridge::read_ram(u16 addr) {
    if (EXTERNAL_RAM_ADDR > addr || addr >= EXTERNAL_RAM_ADDR + RAM_BANK_SIZE) {
        fatal("Invalid ram address read: %04x\n", addr);
    }
    if (ramg) {
        return (*activeRamBank)[addr - EXTERNAL_RAM_ADDR];
    }
    return 0xFF;
}

ROMOnly::ROMOnly(const char* filePath, u8* rom) : Cartridge("ROM Only", 2, 0, filePath, rom) {}

void ROMOnly::write(u16 addr, u8 val) {
    printf("Warning: Attempting to write to rom only cartridge, addr=%02x val=%02x.\n", addr, val);
}

MBC1::MBC1(const char* filePath, u8* rom) : Cartridge("MBC1", 128, 4, filePath, rom) {}

void MBC1::write(u16 addr, u8 val) {
    if (addr < 0x2000) {
        if (hasRam) {
            ramg = (val & 0xF) == 0xA;
        }
    } else if (addr < 0x4000) {
        bank1Num = val & (0x20 - 1);
        if (bank1Num == 0) bank1Num = 1;

        update_banks();
    } else if (addr < 0x6000) {
        bank2Num = val & 0x3;

        update_banks();
    } else if (addr < 0x8000) {
        mode = val & 0x1;

        update_banks();
    } else if (ramg) {
        (*activeRamBank)[addr - EXTERNAL_RAM_ADDR] = val;
    } else if (EXTERNAL_RAM_ADDR > addr || addr >= EXTERNAL_RAM_ADDR + RAM_BANK_SIZE) {
        fatal("Invalid MBC1 address write: %02x\n", addr);
    }
}

void MBC1::update_banks() {
    if (numRomBanks < 64 || !mode) {
        zeroBank = &romBanks[0];
    } else if (numRomBanks == 128) {
        zeroBank = &romBanks[bank2Num << 5];
    } else if (numRomBanks == 64) {
        zeroBank = &romBanks[(bank2Num & 0x1) << 5];
    } else {
        fatal("Invalid number of roms for mbc1: %d", numRomBanks);
    }

    u8 adjustedBank1 = bank1Num & (numRomBanks - 1);
    if (numRomBanks < 64) {
        highBank = &romBanks[adjustedBank1];
    } else if (numRomBanks == 128) {
        highBank = &romBanks[(bank2Num << 5) | adjustedBank1];
    } else if (numRomBanks == 64) {
        highBank = &romBanks[((bank2Num & 0x1) << 5) | adjustedBank1];
    } else {
        fatal("Invalid number of roms for mbc1: %d", numRomBanks);
    }

    if (numRamBanks != MAX_RAM_BANKS || !mode) {
        activeRamBank = &ramBanks[0];
    } else {
        activeRamBank = &ramBanks[bank2Num];
    }
}

MBC5::MBC5(const char* filePath, u8* rom) : Cartridge("MBC5", 512, 16, filePath, rom) {}

void MBC5::write(u16 addr, u8 val) {
    if (addr < 0x2000) {
        if (hasRam) {
            ramg = val == 0xA;
        }
    } else if (addr < 0x3000) {
        romb0 = val;
        highBank = &romBanks[(size_t)((romb1 << 8) | romb0) & (numRomBanks - 1)];
    } else if (addr < 0x4000) {
        romb1 = val & 0x1;
        highBank = &romBanks[(size_t)((romb1 << 8) | romb0) & (numRomBanks - 1)];
    } else if (addr < 0x6000) {
        activeRamBank = &ramBanks[val & 0xF];
    } else if (EXTERNAL_RAM_ADDR > addr || addr >= EXTERNAL_RAM_ADDR + RAM_BANK_SIZE) {
        fatal("Invalid MBC5 address write: %02x\n", addr);
    } else if (ramg) {
        (*activeRamBank)[addr - EXTERNAL_RAM_ADDR] = val;
    }
}
