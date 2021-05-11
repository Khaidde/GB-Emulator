#include "cartridge.hpp"

#include <cstdio>

ROMOnly::ROMOnly(u8* rom) : Cartridge(CartridgeType::ROM_ONLY) {
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

MBC1::MBC1(u8* rom, u8 numRomBanks, u8 numRamBanks) : Cartridge(CartridgeType::ROM_ONLY) {
    if (numRomBanks > MAX_ROM_BANKS) {
        printf("Incompatible number of rom banks for MBC1: %d\n", numRomBanks);
        return;
    }
    this->numRomBanks = numRomBanks;
    this->numRamBanks = numRamBanks;

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
    this->mode = 0;
}

MBC1::~MBC1() {
    if (hasBattery) {
        printf("TODO Saving game...\n");
    }
    delete romBanks;
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