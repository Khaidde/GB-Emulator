#pragma once

#include "utils.hpp"

class Cartridge {
   public:
    Cartridge(const char* cartName, u8* rom);
    virtual ~Cartridge() = default;
    virtual u8 read(u16 addr) = 0;
    virtual void write(u16 addr, u8 val) = 0;

    void setHasRam(bool cartHasRam) { this->hasRam = cartHasRam; }
    void setHasBattery(bool cartHasBattery) { this->hasBattery = cartHasBattery; }
    void setHasTimer(bool cartHasTimer) { this->hasTimer = cartHasTimer; }

    u8 read_header(u16 addr);
    void print_cartridge_info();

   protected:
    const char* cartName;

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
    explicit ROMOnly(u8* rom);
    u8 read(u16 addr) override;
    void write(u16 addr, u8 val) override;

   private:
    static constexpr u16 ROM_SIZE = 0x8000;
    u8 rom[ROM_SIZE];
};

class MBC1 : public Cartridge {
   public:
    MBC1(u8* rom);
    ~MBC1() override;
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