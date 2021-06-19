#include "cartridge.hpp"

#include <string>

namespace {
constexpr const char* ROM_SIZES[] = {
    "32 KByte (no ROM banking)",
    "64 KByte (4 banks)",
    "128 KByte (8 banks)",
    "256 KByte (16 banks)",
    "512 KByte (32 banks)",
    "1 MByte (64 banks or 63 on MBC1)",
    "2 MByte (128 banks or 127 on MBC1)",
    "4 MByte (256 banks)",
    "8 MByte (512 banks)",
};
constexpr struct {
    const char* desc;
    char numBanks;
} RAM_SIZES[] = {
    {"None", 0},
    {"2 KBytes", 1},
    {"8 KBytes", 1},
    {"32 KBytes (4 banks of 8KBytes)", 4},
    {"128 KBytes (16 banks of 8KBytes)", 16},
    {"64 KBytes (8 banks of 8KBytes)", 8},
};
}  // namespace

Cartridge::Cartridge(const char* cartName, u8* rom) : cartName(cartName) {
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

    ramType = rom[0x149];
    if (ramType <= 0x5) {
        if (ramType == 1) {
            fatal("TODO check that 2KByte ram works correctly\n");
        }
        numRamBanks = RAM_SIZES[ramType].numBanks;
    } else {
        fatal("Unknown ram size: %d\n", ramType);
    }
}

void Cartridge::print_cartridge_info() {
    char title[15];
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
    } else {
        printf("CGB Flag: %02x\n", read_header(0x143));
    }

    printf("ROM Size: %s\n", ROM_SIZES[romType]);
    printf("RAM Size: %s\n", RAM_SIZES[ramType].desc);

    std::string cartridgeType(cartName);
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

ROMOnly::ROMOnly(u8* rom) : Cartridge("ROM Only", rom) {
    for (int i = 0; i < ROM_SIZE; i++) {
        this->rom[i] = rom[i];
    }
}

u8 ROMOnly::read(u16 addr) {
    if (addr > 0x8000) {
        printf("Warning: Rom only cartridge has no external ram.\n");
    }
    return rom[addr];
}

void ROMOnly::write(u16 addr, u8 val) {
    printf("Warning: Attempting to write to rom only cartridge, addr=%02x val=%02x.\n", addr, val);
}

MBC1::MBC1(u8* rom) : Cartridge("MBC1", rom) {
    if (numRomBanks > MAX_ROM_BANKS) {
        printf("Incompatible number of rom banks for MBC1: %d\n", numRomBanks);
        return;
    }

    this->romBanks = new RomBank[numRomBanks];
    for (int i = 0; i < numRomBanks; i++) {
        for (int k = 0; k < ROM_BANK_SIZE; k++) {
            romBanks[i][k] = rom[i * ROM_BANK_SIZE + k];
        }
    }

    this->zeroBank = &romBanks[0];

    this->bank1Num = 1;
    this->highBank = &romBanks[bank1Num];

    this->ramBanks = new RamBank[numRamBanks];
    this->bank2Num = 0;
    this->activeRamBank = &ramBanks[0];

    this->ramg = false;
    this->mode = false;
}

MBC1::~MBC1() {
    if (hasBattery) {
        printf("TODO Saving game...\n");
    }
    delete[] romBanks;
    delete[] ramBanks;
}

u8 MBC1::read(u16 addr) {
    if (addr < ROM_BANK_SIZE) {
        return (*zeroBank)[addr];
    } else if (addr < ROM_BANK_SIZE * 2) {
        return (*highBank)[addr & (0x4000 - 1)];
    } else if (ramg && hasRam) {
        return (*activeRamBank)[addr - EXTERNAL_RAM_ADDR];
    } else if (EXTERNAL_RAM_ADDR > addr || addr >= EXTERNAL_RAM_ADDR + RAM_BANK_SIZE) {
        fatal("Invalid MBC1 address read: %02x\n", addr);
    }
    return 0xFF;
}

void MBC1::write(u16 addr, u8 val) {
    if (addr < 0x2000) {
        ramg = (val & 0xF) == 0xA;
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
