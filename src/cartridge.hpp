#pragma once

#include "general.hpp"

struct CartridgeInfo {
    std::string savePath;
    bool isCGB;
    bool cgbMode;  // 1 - CGB-Mode, 0 - DMG-Mode

    bool hasRam = false;
    bool hasBattery = false;
    bool hasTimer = false;
    bool hasRumble = false;  // TODO unimplemented
};

class Cartridge {
public:
    Cartridge(CartridgeInfo& info, const char* cartName, int maxRomBanks, int maxRamBanks, u8* rom);
    virtual ~Cartridge();

    void try_load_save_file();

    virtual void write(u16 addr, u8 val) = 0;

    u8 read_rom(u16 addr);
    virtual u8 read_ram(u16 addr);

    virtual void emulate_cycle();

    bool is_CGB() { return info.isCGB; }
    bool is_CGB_mode() { return info.isCGB && info.cgbMode; }
    bool is_DMG_mode() { return info.isCGB && !info.cgbMode; }

protected:
    CartridgeInfo info;

    static constexpr u16 ROM_BANK_SIZE = 0x4000;
    static constexpr u16 RAM_BANK_SIZE = 0x2000;
    static constexpr u16 EXTERNAL_RAM_ADDR = 0xA000;

    using RomBank = u8[ROM_BANK_SIZE];
    using RamBank = u8[RAM_BANK_SIZE];

    size_t numRomBanks;
    size_t numRamBanks;

    RomBank* romBanks;
    RamBank* ramBanks;

    RomBank* zeroBank;
    RomBank* highBank;

    RamBank* activeRamBank;
    bool ramg = false;  // RAMG - ram gate register, enable or disable ram
};

class ROMOnly : public Cartridge {
public:
    ROMOnly(CartridgeInfo& info, u8* rom);
    void write(u16 addr, u8 val) override;
};

class MBC1 : public Cartridge {
public:
    MBC1(CartridgeInfo& info, u8* rom);
    void write(u16 addr, u8 val) override;

private:
    void update_banks();

private:
    u8 bank1Num = 1;  // BANK1 register for 0x4000-0x7FFF access
    u8 bank2Num = 0;  // BANK2 register for upper bit of ROM bank number or 2-bit RAM bank number
    bool mode = false;
};

class MBC2 : public Cartridge {
public:
    MBC2(CartridgeInfo& info, u8* rom);
    void write(u16 addr, u8 val) override;
    u8 read_ram(u16 addr) override;
};

class MBC3 : public Cartridge {
public:
    MBC3(CartridgeInfo& info, u8* rom);
    void write(u16 addr, u8 val) override;
    u8 read_ram(u16 addr) override;

    void emulate_cycle() override;

private:
    bool isTimerEnabled;

    // TODO only works on little-endian machines
    union Time {
        struct {
            u8 seconds;
            u8 minutes;
            u8 hours;
            u8 dayLo;
            u8 dayHi;
        };
        u8 regs[5];
    };
    static constexpr Time timeBitmasks = {0x3F, 0x3F, 0x1F, 0xFF, 0xC1};

    bool isRTCSelected = false;
    bool isLatchHigh = false;
    u8 curRTCRegNum = 0;
    Time latchedTime;

    static constexpr u32 CYCLES_PER_SECOND = 4194304 / 4;  // 4194304 clocks per second
    u32 cycles;
    Time realTime;

    bool rtcOn = false;
};

class MBC5 : public Cartridge {
public:
    MBC5(CartridgeInfo& info, u8* rom);
    void write(u16 addr, u8 val) override;

private:
    u8 romb0 = 1;  // lower 8-bits of bank for 0x4000-0x7FFF access
    u8 romb1 = 0;  // upper 9th bit of bank for 0x4000-0x7FFF access
};
