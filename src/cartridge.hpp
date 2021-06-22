#pragma once

#include "general.hpp"

class Cartridge {
public:
    Cartridge(const char* cartName, size_t maxRomBanks, size_t maxRamBanks, const char* filePath,
              u8* rom);
    virtual ~Cartridge();

    void load_save_file(const char* savePath);

    virtual void write(u16 addr, u8 val) = 0;

    u8 read_rom(u16 addr);
    u8 read_ram(u16 addr);

    bool is_CGB() { return isCGB; }

    void set_has_ram(bool cartHasRam) { this->hasRam = cartHasRam; }
    void set_has_battery(bool cartHasBattery) { this->hasBattery = cartHasBattery; }
    void set_has_timer(bool cartHasTimer) { this->hasTimer = cartHasTimer; }

    void print_cartridge_info();

private:
    u8 read_header(u16 addr);

protected:
    bool isCGB;

    const char* cartridgeName;
    const char* filePath;

    static constexpr u16 HEADER_START = 0x104;
    static constexpr u16 HEADER_SIZE = 0x150 - HEADER_START;
    u8 header[HEADER_SIZE];

    bool hasRam = false;
    bool hasBattery = false;
    bool hasTimer = false;   // TODO unimplemented
    bool hasRumble = false;  // TODO unimplemented

    static constexpr u16 ROM_BANK_SIZE = 0x4000;
    static constexpr u16 RAM_BANK_SIZE = 0x2000;
    static constexpr u16 EXTERNAL_RAM_ADDR = 0xA000;

    using RomBank = u8[ROM_BANK_SIZE];
    using RamBank = u8[RAM_BANK_SIZE];

    u8 romType;
    u8 ramType;
    size_t numRomBanks;
    size_t numRamBanks;
    const size_t MAX_ROM_BANKS;
    const size_t MAX_RAM_BANKS;

    RomBank* romBanks;
    RamBank* ramBanks;

    RomBank* zeroBank;
    RomBank* highBank;

    RamBank* activeRamBank;
    bool ramg;  // RAMG - ram gate register, enable or disable ram
};

class ROMOnly : public Cartridge {
public:
    ROMOnly(const char* filePath, u8* rom);
    void write(u16 addr, u8 val) override;
};

class MBC1 : public Cartridge {
public:
    MBC1(const char* filePath, u8* rom);
    void write(u16 addr, u8 val) override;

private:
    void update_banks();

private:
    u8 bank1Num = 1;  // BANK1 register for 0x4000-0x7FFF access
    u8 bank2Num = 0;  // BANK2 register for upper bit of ROM bank number or 2-bit RAM bank number
    bool mode = false;
};

class MBC5 : public Cartridge {
public:
    MBC5(const char* filePath, u8* rom);
    void write(u16 addr, u8 val) override;

private:
    u8 romb0 = 1;  // lower 8-bits of bank for 0x4000-0x7FFF access
    u8 romb1 = 0;  // upper 9th bit of bank for 0x4000-0x7FFF access
};
