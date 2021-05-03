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
    if (!get_lcdc_flag(LCD_ENABLE)) {
        drawClocks = 0;
        mode = OAM_SEARCH;
        return;
    }
    drawClocks++;
    switch (mode) {
        case OAM_SEARCH:
            if (drawClocks >= OAM_SEARCH_CLOCKS) {
                drawClocks -= OAM_SEARCH_CLOCKS;

                spriteList.clear();
                for (int i = 0; i < 40; i++) {
                    u16 addr = get_sprite_addr(i);
                    u8 y = memory->read(addr);
                    u8 x = memory->read(addr + 1);

                    if (x > 0 && *ly + 16 >= y && *ly + 16 < y + (get_lcdc_flag(SPRITE_SIZE) ? 16 : 8)) {
                        spriteList.add({y, x, memory->read(addr + 2), memory->read(addr + 3)});
                        if (spriteList.size == SpriteList::MAX_SPRITES) break;
                    }
                }

                fetcher.doFetch = false;
                fetcher.fetchState = Fetcher::READ_TILE_ID;
                fetcher.curSprite = nullptr;
                fetcher.windowMode = false;
                fetcher.tileX = 0;
                numDiscardedPixels = 0;
                curPixelX = 0;

                bgFifo.clear();

                mode = LCD_TRANSFER;
                // TODO prohibit cpu vram access
            }
            break;
        case LCD_TRANSFER: {
            if (bgFifo.size > 8 && !fetcher.curSprite) {
                if (numDiscardedPixels < *scx % TILE_PX_SIZE) {
                    numDiscardedPixels++;
                    bgFifo.pop();
                } else {
                    if (!fetcher.windowMode && get_lcdc_flag(WINDOW_ENABLE) && get_lcdc_flag(BG_WINDOW_ENABLE) &&
                        *ly == *wy && curPixelX >= *wx - 7) {
                        fetcher.windowMode = true;
                        printf("TODO window mode not implemented yet\n");
                    } else {
                        if (get_lcdc_flag(SPRITE_ENABLE)) {
                            for (int i = 0; i < spriteList.size; i++) {
                                // Check if sprite has already been handled
                                if ((spriteList.removedBitField & (1 << i)) != 0) continue;

                                Sprite* sprite = &spriteList.data[i];
                                if (sprite->x <= curPixelX + 8) {
                                    fetcher.doFetch = false;
                                    fetcher.fetchState = Fetcher::READ_TILE_ID;
                                    fetcher.curSprite = sprite;
                                    spriteList.remove(i--);
                                    break;
                                }
                            }
                        }
                        if (!fetcher.curSprite) {
                            frameBuffer[*ly * GameBoy::WIDTH + curPixelX++] = get_color(bgFifo.pop());
                        }
                    }
                }
            }

            if (fetcher.doFetch) {
                if (fetcher.curSprite) {
                    sprite_fetch();
                } else {
                    background_fetch();
                }
            }
            fetcher.doFetch = !fetcher.doFetch;

            if (curPixelX == GameBoy::WIDTH) {
                // printf("----------------------ly=%d,c=%d\n", *ly, drawClocks);
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

void PPU::background_fetch() {
    switch (fetcher.fetchState) {
        case Fetcher::READ_TILE_ID: {
            u16 tileMap = (fetcher.windowMode ? get_lcdc_flag(WINDOW_TILE_SELECT) : get_lcdc_flag(BG_TILE_SELECT))
                              ? 0x9C00
                              : 0x9800;

            s8 tileIndex;
            if (fetcher.windowMode) {
                printf("TODO window mode not implemented yet\n");
                tileIndex = 0;
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
            fetcher.fetchState = Fetcher::READ_TILE_0;
        } break;
        case Fetcher::READ_TILE_0:
            fetcher.data0 = memory->read(fetcher.tileRowAddr);
            fetcher.fetchState = Fetcher::READ_TILE_1;
            break;
        case Fetcher::READ_TILE_1:
            fetcher.data1 = memory->read(fetcher.tileRowAddr + 1);
            if (bgFifo.size >= 8) {
                fetcher.fetchState = Fetcher::PUSH;
            }
        case Fetcher::PUSH:
            if (bgFifo.size <= 8) {
                if (get_lcdc_flag(BG_WINDOW_ENABLE)) {
                    for (int i = 0; i < TILE_PX_SIZE; i++) {
                        u8 align = (1 << 7) >> i;

                        bool hi = fetcher.data1 & align;
                        bool lo = fetcher.data0 & align;
                        u8 col = (hi << 1) | lo;
                        bgFifo.push({col, false, Memory::BGP_REG});
                    }
                } else {
                    for (int i = 0; i < TILE_PX_SIZE; i++) {
                        bgFifo.push({0, false, Memory::BGP_REG});
                    }
                }
                fetcher.tileX++;
                fetcher.fetchState = Fetcher::READ_TILE_ID;
            }
            break;
        default:
            break;
    };
}

void PPU::sprite_fetch() {
    switch (fetcher.fetchState) {
        case Fetcher::READ_TILE_ID: {
            u8 tileIndex = fetcher.curSprite->tileID;
            if (get_lcdc_flag(SPRITE_SIZE)) {
                printf("TODO test tall sprite mode");
                tileIndex &= ~0x1;  // Make tileIndex even
            }

            u8 tileByteOff;
            bool yFlip = fetcher.curSprite->flags & (1 << 6);
            if (yFlip) {
                tileByteOff = (fetcher.curSprite->y - *ly - 0x9) * 2;
            } else {
                tileByteOff = (0x10 - (fetcher.curSprite->y - *ly)) * 2;
            }

            fetcher.tileRowAddr = VRAM_TILE_DATA_0 + tileIndex * TILE_MEM_LEN + tileByteOff;
            fetcher.fetchState = Fetcher::READ_TILE_0;
        } break;
        case Fetcher::READ_TILE_0:
            fetcher.data0 = memory->read(fetcher.tileRowAddr);
            fetcher.fetchState = Fetcher::READ_TILE_1;
            break;
        case Fetcher::READ_TILE_1:
            fetcher.data1 = memory->read(fetcher.tileRowAddr + 1);

        case Fetcher::PUSH: {
            int xOff = fetcher.curSprite->x < 8 ? 8 - fetcher.curSprite->x : 0;
            bool bgPriority = fetcher.curSprite->flags & (1 << 7);
            bool isObjectPalette1 = fetcher.curSprite->flags & (1 << 4);
            for (int i = 0; i < TILE_PX_SIZE - xOff; i++) {
                bool xFlip = fetcher.curSprite->flags & (1 << 5);
                u8 align;
                if (xFlip) {
                    align = 1 << (i + xOff);
                } else {
                    align = (1 << 7) >> (i + xOff);
                }

                bool hi = fetcher.data1 & align;
                bool lo = fetcher.data0 & align;
                u8 col = (hi << 1) | lo;

                if (col != 0 && !bgFifo.get(i)->isSprite) {
                    if (!bgPriority || bgFifo.get(i)->colIndex == 0) {
                        if (isObjectPalette1) {
                            bgFifo.set(i, {col, true, Memory::OBP1_REG});
                        } else {
                            bgFifo.set(i, {col, true, Memory::OBP0_REG});
                        }
                    }
                }
            }
            fetcher.curSprite = nullptr;
            fetcher.fetchState = Fetcher::READ_TILE_ID;
        } break;
    }
}

bool PPU::get_lcdc_flag(LCDCFlag flag) { return memory->read(Memory::LCDC_REG) & (1 << flag); }

u32 PPU::get_color(FIFOData&& data) {
    data.colIndex &= 0x03;

    u8 i = (data.colIndex << 1);
    u8 mask = 3 << i;

    return baseColors[(memory->read(data.palleteAddr) & mask) >> i];
}

u16 PPU::get_sprite_addr(u8 index) {
    static constexpr u16 OAM_ADDR = 0xFE00;
    return OAM_ADDR + (index << 2);
}