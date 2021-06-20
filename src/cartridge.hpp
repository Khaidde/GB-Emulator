#pragma once

#include <vector>

#include "general.hpp"

class Cartridge {
public:
    Cartridge(const char* cartName, const char* filePath, u8* rom);
    virtual ~Cartridge() = default;
    virtual void load_save_ram(u8* ram){};
    void save_to_file(std::vector<u8>& ram);
    virtual u8 read(u16 addr) = 0;
    virtual void write(u16 addr, u8 val) = 0;

    void set_has_ram(bool cartHasRam) { this->hasRam = cartHasRam; }
    void set_has_battery(bool cartHasBattery) { this->hasBattery = cartHasBattery; }
    void set_has_timer(bool cartHasTimer) { this->hasTimer = cartHasTimer; }

    void print_cartridge_info();

private:
    u8 read_header(u16 addr);

protected:
    const char* cartridgeName;
    const char* filePath;

    static constexpr u16 HEADER_START = 0x104;
    static constexpr u16 HEADER_SIZE = 0x150 - HEADER_START;
    u8 header[HEADER_SIZE];

    bool hasRam = false;
    bool hasBattery = false;
    bool hasTimer = false;   // TODO unimplemented
    bool hasRumble = false;  // TODO unimplemented

    u8 romType;
    u8 ramType;
    u16 numRomBanks;
    u16 numRamBanks;
};

class ROMOnly : public Cartridge {
public:
    ROMOnly(const char* filePath, u8* rom);
    u8 read(u16 addr) override;
    void write(u16 addr, u8 val) override;

private:
    static constexpr u16 ROM_SIZE = 0x8000;
    u8 rom[ROM_SIZE];
};

class MBC1 : public Cartridge {
public:
    MBC1(const char* filePath, u8* rom);
    ~MBC1() override;
    void load_save_ram(u8* ram) override;
    u8 read(u16 addr) override;
    void write(u16 addr, u8 val) override;
    void update_banks();

private:
    static constexpr u16 ROM_BANK_SIZE = 0x4000;
    using RomBank = u8[ROM_BANK_SIZE];
    static constexpr u8 MAX_ROM_BANKS = 128;
    RomBank* romBanks;

    RomBank* zeroBank;

    u8 bank1Num;  // BANK1 register for 0x4000 -0x7FFF access
    RomBank* highBank;

    u8 bank2Num;  // BANK2 register for upper bit of ROM bank number or 2-bit RAM bank number

    static constexpr u16 EXTERNAL_RAM_ADDR = 0xA000;
    static constexpr u16 RAM_BANK_SIZE = 0x2000;
    using RamBank = u8[RAM_BANK_SIZE];
    static constexpr u8 MAX_RAM_BANKS = 4;
    RamBank* ramBanks;

    RamBank* activeRamBank;

    bool ramg;  // RAMG - ram gate register, enable or disable ram
    bool mode;
};

class MBC5 : public Cartridge {
public:
    MBC5(const char* filePath, u8* rom);
    ~MBC5() override;
    u8 read(u16 addr) override;
    void write(u16 addr, u8 val) override;
    void update_banks();

private:
    static constexpr u8 MAX_ROM_BANKS = 128;
};
