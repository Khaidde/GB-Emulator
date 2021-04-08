#include "ppu.hpp"

#include <stdio.h>

#include "game_boy.hpp"

PPU::PPU(Memory* memory) : memory(memory) {
    frameBuffer = new u32[GameBoy::WIDTH * GameBoy::HEIGHT];
    for (int i = 0; i < GameBoy::WIDTH * GameBoy::HEIGHT; i++) {
        frameBuffer[i] = 0xFFFF00FF;
    }
}

PPU::~PPU() { delete[] frameBuffer; }

void PPU::render(u32* pixelBuffer) {
    if (memory->read(Memory::LCDC_REG) & (1 << 7)) {
        for (int i = 0; i < GameBoy::WIDTH * GameBoy::HEIGHT; i++) {
            pixelBuffer[i] = frameBuffer[i];
        }
    } else {
        for (int i = 0; i < GameBoy::WIDTH * GameBoy::HEIGHT; i++) {
            // TODO temp hardcoded color "whiter than black"
            pixelBuffer[i] = BLANK_COLOR;
        }
    }
}

void PPU::proccess(u8 cycles) {
    if (memory->read(Memory::LCDC_REG) & (1 << 7)) {
        drawCycles += cycles;
        switch (mode) {
            case OAM_SEARCH:
                if (drawCycles >= OAM_SEARCH_CYCLES) {
                    drawCycles -= OAM_SEARCH_CYCLES;
                    mode = LCD_TRANSFER;
                }
                break;
            case LCD_TRANSFER:
                if (drawCycles >= LCD_TRANSFER_CYCLES) {
                    drawCycles -= LCD_TRANSFER_CYCLES;
                    mode = H_BLANK;
                    draw_bg(memory->read(Memory::LY_REG));
                }
                break;
            case H_BLANK:
                if (drawCycles >= H_BLANK_CYCLES) {
                    drawCycles -= H_BLANK_CYCLES;
                    memory->inc(Memory::LY_REG);

                    if (memory->read(Memory::LY_REG) >= GameBoy::HEIGHT) {
                        // memory->request_interrupt(Memory::VBLANK_INT);
                        mode = V_BLANK;
                    } else {
                        mode = OAM_SEARCH;
                    }
                }
                break;
            case V_BLANK:
                if (drawCycles >= SCAN_LINE_CYCLES) {
                    drawCycles -= SCAN_LINE_CYCLES;
                    memory->inc(Memory::LY_REG);

                    if (memory->read(Memory::LY_REG) >= V_BLANK_END_LINE) {
                        mode = OAM_SEARCH;
                        memory->write(Memory::LY_REG, 0);
                    }
                }
                break;
        }
    }
}

u32 PPU::get_color(u8 col) {
    col &= 0x03;

    u8 i = col * 2;
    u8 mask = i | (i << 1);

    return baseColors[(memory->read(Memory::BGP_REG) & mask) >> i];
}

void PPU::draw_bg(u8 line) {
    bool unsign = memory->read(Memory::LCDC_REG) & (1 << 4);

    u16 bgTileMap = (memory->read(Memory::LCDC_REG) & (1 << 3)) ? 0x9C00 : 0x9800;

    u8 tileByteOff = (line % 8) * 2;
    u8 tileX;
    u8 tileY = (line / 8);  // TODO add scroll y
    for (int x = 0; x < GameBoy::WIDTH; x++) {
        tileX = (x / 8);  // TODO add scroll x

        u16 tileAddr;
        if (unsign) {
            u8 tileIndex = memory->read((tileX + tileY * 32) + bgTileMap);
            if (tileIndex != 0) {
                // printf("%02x\n", tileIndex);
            }
            /*
            if (tileX == 8 && tileY == 9) {
            }
            */
            tileAddr = 0x8000 + tileIndex * 16;
        } else {
            s8 tileIndex = memory->read((tileX + tileY * 32) + bgTileMap);
            tileAddr = 0x9000 + tileIndex * 16;
        }

        u8 tileUpper = memory->read(tileAddr + tileByteOff);
        u8 tileLower = memory->read(tileAddr + tileByteOff + 1);

        u8 align = (1 << (7 - x % 8));
        u8 lo = (tileUpper & align) != 0;
        u8 hi = (tileLower & align) != 0;
        u8 col = (hi << 1) | lo;

        frameBuffer[line * GameBoy::WIDTH + x] = get_color(col);
    }
}