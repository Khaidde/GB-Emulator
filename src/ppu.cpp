#include "ppu.hpp"

#include <stdio.h>

#include "game_boy.hpp"

void PPU::init(Memory* memory) {
    this->memory = memory;
    ly = &memory->ref(Memory::LY_REG);
    scy = &memory->ref(Memory::SCY_REG);
    scx = &memory->ref(Memory::SCX_REG);
    wy = &memory->ref(Memory::WY_REG);
    wx = &memory->ref(Memory::WX_REG);

    for (int i = 0; i < GameBoy::WIDTH * GameBoy::HEIGHT; i++) {
        frameBuffer[i] = BLANK_COLOR;
    }
    drawClocks = 0;
}

void PPU::render(u32* pixelBuffer) {
    for (int i = 0; i < GameBoy::WIDTH * GameBoy::HEIGHT; i++) {
        pixelBuffer[i] = get_lcdc_flag(LCD_ENABLE) ? frameBuffer[i] : BLANK_COLOR;
    }
}

void PPU::emulate_clock() {
    if (get_lcdc_flag(LCD_ENABLE)) {
        drawClocks++;
        switch (mode) {
            case OAM_SEARCH:
                if (drawClocks >= OAM_SEARCH_CLOCKS) {
                    drawClocks -= OAM_SEARCH_CLOCKS;

                    spriteBag.clear();
                    for (int i = 0; i < 40; i++) {
                        u16 addr = get_sprite_addr(i);
                        u8 y = memory->read(addr);
                        u8 x = memory->read(addr + 1);

                        if (x > 0 && *ly + 16 >= y && *ly + 16 < y + (get_lcdc_flag(SPRITE_SIZE) ? 16 : 8)) {
                            spriteBag.add({y, x, memory->read(addr + 2), memory->read(addr + 3)});
                            if (spriteBag.size == SpriteBag::MAX_SPRITES) break;
                        }
                    }

                    fetcher.clocks = 0;
                    fetcher.windowMode = false;
                    fetcher.tileX = 0;
                    fetcher.curSprite = nullptr;
                    numDiscardedPixels = 0;
                    curPixelX = 0;

                    bgFifo.clear();
                    spriteFifo.clear();

                    mode = LCD_TRANSFER;
                    // TODO prohibit cpu vram access
                }
                break;
            case LCD_TRANSFER: {
                if (fetcher.curSprite) {
                    sprite_fetch();
                    fetcher.clocks = (fetcher.clocks + 1) % 8;
                    return;
                } else {
                    background_fetch();
                    fetcher.clocks = (fetcher.clocks + 1) % 8;
                    if (bgFifo.size <= 8) return;
                }

                if (numDiscardedPixels < *scx % TILE_PX_SIZE) {
                    numDiscardedPixels++;
                    bgFifo.pop();
                    return;
                }

                if (!fetcher.windowMode && get_lcdc_flag(WINDOW_ENABLE) && get_lcdc_flag(BG_WINDOW_ENABLE)) {
                    if (*ly == *wy && curPixelX >= *wx - 7) {
                        fetcher.windowMode = true;
                        printf("TODO window mode not implemented yet\n");
                        return;
                    }
                }

                if (get_lcdc_flag(SPRITE_ENABLE) && !fetcher.curSprite) {
                    for (int i = 0; i < spriteBag.size; i++) {
                        Sprite* sprite = &spriteBag.data[i];
                        if (sprite->x <= curPixelX + 8) {
                            fetcher.clocks = 0;
                            fetcher.curSprite = sprite;
                            spriteBag.remove(i--);
                            break;
                        }
                    }
                }

                frameBuffer[*ly * GameBoy::WIDTH + curPixelX++] = get_color(bgFifo.pop());

                if (curPixelX == GameBoy::WIDTH) {
                    // printf("c=%d\n", drawClocks);
                    mode = H_BLANK;
                }
            } break;
            case H_BLANK:
                if (drawClocks >= H_BLANK_CLOCKS + LCD_TRANSFER_CLOCKS) {
                    drawClocks -= H_BLANK_CLOCKS + LCD_TRANSFER_CLOCKS;
                    memory->inc(Memory::LY_REG);

                    if (memory->read(Memory::LY_REG) >= GameBoy::HEIGHT) {
                        memory->request_interrupt(Memory::VBLANK_INT);
                        mode = V_BLANK;
                    } else {
                        mode = OAM_SEARCH;
                    }
                }
                break;
            case V_BLANK:
                if (drawClocks >= SCAN_LINE_CLOCKS) {
                    drawClocks -= SCAN_LINE_CLOCKS;
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

void PPU::background_fetch() {
    switch (fetcher.clocks) {
        case 0: {
            u16 tileMap = (fetcher.windowMode ? get_lcdc_flag(WINDOW_TILE_SELECT) : get_lcdc_flag(BG_TILE_SELECT))
                              ? 0x9C00
                              : 0x9800;

            s8 tileIndex;
            if (fetcher.windowMode) {
                printf("TODO window mode not implemented yet\n");
            } else {
                u8 tileX = (*scx / TILE_PX_SIZE + fetcher.tileX) % TILESET_SIZE;
                u8 tileY = ((*scy + *ly) / TILE_PX_SIZE) % TILESET_SIZE;
                tileIndex = memory->read((tileX + tileY * TILESET_SIZE) + tileMap);
            }

            u8 tileByteOff = ((*ly + *scy) % TILE_PX_SIZE) * 2;
            bool unsign = get_lcdc_flag(TILE_DATA_SELECT);
            if (unsign) {
                fetcher.tileRowAddr = VRAM_TILE_DATA_0 + (u8)tileIndex * TILE_MEM_LEN + tileByteOff;
            } else {
                fetcher.tileRowAddr = VRAM_TILE_DATA_1 + tileIndex * TILE_MEM_LEN + tileByteOff;
            }
            break;
        }
        case 2:
            fetcher.data0 = memory->read(fetcher.tileRowAddr);
            break;
        case 4:
            fetcher.data1 = memory->read(fetcher.tileRowAddr + 1);
            if (bgFifo.size < 8) {
                fetcher.clocks += 2;
            } else {
                break;
            }
        case 6:
            if (get_lcdc_flag(BG_WINDOW_ENABLE)) {
                for (int i = 0; i < TILE_PX_SIZE; i++) {
                    u8 align = (1 << 7) >> i;

                    bool hi = fetcher.data1 & align;
                    bool lo = fetcher.data0 & align;
                    u8 col = (hi << 1) | lo;
                    bgFifo.push(col);
                }
            } else {
                for (int i = 0; i < TILE_PX_SIZE; i++) {
                    bgFifo.push(0);
                }
            }
            fetcher.tileX++;
            break;
        default:
            break;
    };
}

void PPU::sprite_fetch() {
    switch (fetcher.clocks) {
        case 0: {
            u8 tileIndex = fetcher.curSprite->tileID;
            if (get_lcdc_flag(SPRITE_SIZE)) {
                printf("TODO test tall sprite mode");
                tileIndex &= ~0x1;  // Make tileIndex even
            }

            u8 tileByteOff = (0x10 - (fetcher.curSprite->y - *ly)) * 2;
            fetcher.tileRowAddr = VRAM_TILE_DATA_0 + tileIndex * TILE_MEM_LEN + tileByteOff;

            for (int i = 0; i < 4; i++) {
            }
            printf("%02x-%02x-%02x-%02x", fetcher.curSprite->y, fetcher.curSprite->x, fetcher.curSprite->tileID,
                   fetcher.curSprite->priority);
            printf("===%02x:%04x:%02x\n", tileIndex, fetcher.tileRowAddr, *ly);
        } break;
        case 2:
            fetcher.data0 = memory->read(fetcher.tileRowAddr);
            break;
        case 4:
            fetcher.data1 = memory->read(fetcher.tileRowAddr + 1);
            break;
        case 6:
            for (int i = 0; i < TILE_PX_SIZE; i++) {
                u8 align = (1 << 7) >> i;

                bool hi = fetcher.data1 & align;
                bool lo = fetcher.data0 & align;
                u8 col = (hi << 1) | lo;
                bgFifo.push(col);
                bgFifo.data[i] = col;
            }
            fetcher.curSprite = nullptr;
            break;
    }
}

bool PPU::get_lcdc_flag(LCDCFlag flag) { return memory->read(Memory::LCDC_REG) & (1 << flag); }

u32 PPU::get_color(u8 col) {
    col &= 0x03;

    u8 i = (col << 1);
    u8 mask = 3 << i;

    return baseColors[(memory->read(Memory::BGP_REG) & mask) >> i];
}

u16 PPU::get_sprite_addr(u8 index) {
    static constexpr u16 OAM_ADDR = 0xFE00;
    return OAM_ADDR + (index << 2);
}