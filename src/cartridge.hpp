#pragma once

#include "utils.hpp"

struct Cartridge {
    void load_from_path(const char* romPath);

    static constexpr u16 CARTRIDGE_SIZE = 0x8000;
    u8 rom[CARTRIDGE_SIZE];
};