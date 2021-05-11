#pragma once

#include "utils.hpp"

enum class CartridgeType {
    ROM_ONLY,
    MBC1,
    MBC2,
    MBC3,
    MBC5,
    MBC6,
    MBC7,
    HuC3,
    HuC1,
};

class Cartridge {
   public:
    Cartridge(CartridgeType type) : cartType(type) {}
    virtual ~Cartridge() {}
    virtual u8 read(u16 addr) = 0;
    virtual void write(u16 addr, u8 val) = 0;

    void setHasRam(bool hasRam) { this->hasRam = hasRam; }
    void setHasBattery(bool hasBattery) { this->hasBattery = hasBattery; }
    void setHasTimer(bool hasTimer) { this->hasTimer = hasTimer; }

   protected:
    CartridgeType cartType;

    bool hasRam = false;
    bool hasBattery = false;
    bool hasTimer = false;   // TODO unimplemented
    bool hasRumble = false;  // TODO unimplemented
};

class ROMOnly : public Cartridge {
   public:
    ROMOnly(u8* rom);
    u8 read(u16 addr) override;
    void write(u16 addr, u8 val) override;

   private:
    static constexpr u16 ROM_SIZE = 0x8000;
    u8 rom[ROM_SIZE];
};

class MBC1 : public Cartridge {
   public:
    MBC1(u8* rom, u8 numRomBanks, u8 numRamBanks);
    ~MBC1() override;
    u8 read(u16 addr) override;
    void write(u16 addr, u8 val) override;
    void update_banks();

   private:
    static constexpr u16 ROM_BANK_SIZE = 0x4000;
    using RomBank = u8[ROM_BANK_SIZE];
    static constexpr u8 MAX_ROM_BANKS = 128;
    u8 numRomBanks;
    RomBank* romBanks;

    RomBank* zeroBank;

    u8 bank1Num;  // BANK1 register for 0x4000 -0x7FFF access
    RomBank* highBank;

    u8 bank2Num;  // BANK2 register for upper bit of ROM bank number or 2-bit RAM bank number

    static constexpr u16 EXTERNAL_RAM_ADDR = 0xA000;
    static constexpr u16 RAM_BANK_SIZE = 0x2000;
    using RamBank = u8[RAM_BANK_SIZE];
    static constexpr u8 MAX_RAM_BANKS = 4;
    u8 numRamBanks;
    RamBank* ramBanks;

    RamBank* activeRamBank;

    bool ramg;  // RAMG - ram gate register, enable or disable ram
    bool mode;
};