#include "ppu.hpp"

#include "game_boy.hpp"

void PPU::init(Memory* memory) {
    this->memory = memory;
    lcdc = &memory->ref(IOReg::LCDC_REG);
    stat = &memory->ref(IOReg::STAT_REG);
    scy = &memory->ref(IOReg::SCY_REG);
    scx = &memory->ref(IOReg::SCX_REG);
    ly = &memory->ref(IOReg::LY_REG);
    lyc = &memory->ref(IOReg::LYC_REG);
    wy = &memory->ref(IOReg::WY_REG);
    wx = &memory->ref(IOReg::WX_REG);

    for (int i = 0; i < GameBoy::WIDTH * GameBoy::HEIGHT; i++) {
        frameBuffer[i] = BLANK_COLOR;
    }
    drawClocks = 0;

    statTrigger = false;
}

void PPU::render(u32* pixelBuffer) {
    for (int i = 0; i < GameBoy::WIDTH * GameBoy::HEIGHT; i++) {
        pixelBuffer[i] = get_lcdc_flag(LCDCFlag::LCD_ENABLE) ? frameBuffer[i] : BLANK_COLOR;
    }
}

extern bool test;
void PPU::emulate_clock() {
    if (!get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
        drawClocks = 0;
        mode = Mode::OAM_SEARCH;
        *stat = (*stat & 0xFC) | 2;
        statTrigger = false;
        return;
    }
    if (test && *ly >= 0x8F) {
        if (mode == Mode::OAM_SEARCH) {
            printf("%d\n", (OAM_SEARCH_CLOCKS - drawClocks) / 2);
        }
        if (mode == Mode::V_BLANK) {
            printf("%d\n", (SCAN_LINE_CLOCKS - drawClocks) / 2);
        }
    }
    drawClocks++;
    switch (mode) {
        case Mode::OAM_SEARCH:
            if (drawClocks == 5) {
                *stat = (*stat & 0xFC) | mode;
                update_stat_intr();
            }
            if (drawClocks >= OAM_SEARCH_CLOCKS) {
                drawClocks -= OAM_SEARCH_CLOCKS;

                spriteList.clear();
                for (int i = 0; i < 40; i++) {
                    u16 addr = get_sprite_addr(i);
                    u8 y = memory->read(addr);
                    u8 x = memory->read(addr + 1);

                    if (x > 0 && *ly + 16 >= y && *ly + 16 < y + (get_lcdc_flag(LCDCFlag::SPRITE_SIZE) ? 16 : 8)) {
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

                pxlFifo.clear();

                set_mode(LCD_TRANSFER);
                // TODO prohibit cpu vram access
            }
            break;
        case LCD_TRANSFER: {
            if (drawClocks == 5) {
                *stat = (*stat & 0xFC) | mode;
                update_stat_intr();
            }
            if (pxlFifo.size > 8 && !fetcher.curSprite) {
                if (numDiscardedPixels < *scx % TILE_PX_SIZE) {
                    numDiscardedPixels++;
                    pxlFifo.pop();
                } else {
                    if (!fetcher.windowMode && get_lcdc_flag(LCDCFlag::WINDOW_ENABLE) &&
                        get_lcdc_flag(LCDCFlag::BG_WINDOW_ENABLE) && *ly == *wy && curPixelX >= *wx - 7) {
                        fetcher.windowMode = true;
                        printf("TODO window mode not implemented yet\n");
                    } else {
                        if (get_lcdc_flag(LCDCFlag::SPRITE_ENABLE)) {
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
                            frameBuffer[*ly * GameBoy::WIDTH + curPixelX++] = get_color(pxlFifo.pop());
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
                set_mode(H_BLANK);
            }
        } break;
        case H_BLANK:
            if (drawClocks == 5) {
                *stat = (*stat & 0xFC) | mode;
                update_stat_intr();
            }
            if (drawClocks >= LCD_AND_H_BLANK_CLOCKS) {
                drawClocks -= LCD_AND_H_BLANK_CLOCKS;
                (*ly)++;
                update_coincidence(*lyc);

                if (*ly >= GameBoy::HEIGHT) {
                    memory->request_interrupt(Interrupt::VBLANK_INT);
                    set_mode(V_BLANK);
                } else {
                    set_mode(OAM_SEARCH);
                }
            }
            break;
        case V_BLANK:
            if (drawClocks == 5) {
                if (*ly == GameBoy::HEIGHT) {
                    *stat = (*stat & 0xFC) | mode;
                    update_stat_intr();
                } else if (*ly == V_BLANK_END_LINE - 1) {
                    update_coincidence(*lyc);
                }
            }
            if (drawClocks >= SCAN_LINE_CLOCKS) {
                drawClocks -= SCAN_LINE_CLOCKS;
                (*ly)++;
                update_coincidence(*lyc);

                if (*ly >= V_BLANK_END_LINE) {
                    *ly = 0;
                    update_coincidence(*lyc);
                    set_mode(OAM_SEARCH);
                }
            }
            break;
    }
}

void PPU::background_fetch() {
    switch (fetcher.fetchState) {
        case Fetcher::READ_TILE_ID: {
            u16 tileMap = (fetcher.windowMode ? get_lcdc_flag(LCDCFlag::WINDOW_TILE_SELECT)
                                              : get_lcdc_flag(LCDCFlag::BG_TILE_SELECT))
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
            bool unsign = get_lcdc_flag(LCDCFlag::TILE_DATA_SELECT);
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
            if (pxlFifo.size >= 8) {
                fetcher.fetchState = Fetcher::PUSH;
            }
        case Fetcher::PUSH:
            if (pxlFifo.size <= 8) {
                if (get_lcdc_flag(LCDCFlag::BG_WINDOW_ENABLE)) {
                    for (int i = 0; i < TILE_PX_SIZE; i++) {
                        u8 align = (1 << 7) >> i;

                        bool hi = fetcher.data1 & align;
                        bool lo = fetcher.data0 & align;
                        u8 col = (hi << 1) | lo;
                        pxlFifo.push({col, false, IOReg::BGP_REG});
                    }
                } else {
                    for (int i = 0; i < TILE_PX_SIZE; i++) {
                        pxlFifo.push({0, false, IOReg::BGP_REG});
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
            if (get_lcdc_flag(LCDCFlag::SPRITE_SIZE)) {
                // printf("TODO test tall sprite mode");
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

                if (col != 0 && !pxlFifo.get(i)->isSprite) {
                    if (!bgPriority || pxlFifo.get(i)->colIndex == 0) {
                        if (isObjectPalette1) {
                            pxlFifo.set(i, {col, true, IOReg::OBP1_REG});
                        } else {
                            pxlFifo.set(i, {col, true, IOReg::OBP0_REG});
                        }
                    }
                }
            }
            fetcher.curSprite = nullptr;
            fetcher.fetchState = Fetcher::READ_TILE_ID;
        } break;
    }
}

bool PPU::get_lcdc_flag(LCDCFlag flag) { return *lcdc & (1 << (u8)flag); }

void PPU::set_mode(Mode mode) {
    // When switching from oam_search to v_blank, momentarily switch to h_blank
    if (mode == OAM_SEARCH && this->mode == V_BLANK) {
        *stat = (*stat & 0xFC);
    }
    // TODO only delay stat mode change on DMG
    this->mode = mode;
    if (test) {
        printf("mode switch to %d-ly=%d\n", mode, *ly);
    }
}

void PPU::update_coincidence(u8 lyc) {
    *stat = (*stat & 0xFB) | ((read_ly() == lyc) << 2);
    update_stat_intr();
}

u8 PPU::read_ly() {
    if (*ly == 0x99 && drawClocks > 4) return 0;
    return *ly;
}

void PPU::update_stat_intr() {
    if ((*stat & (1 << 6)) && (read_ly() == *lyc) && !statTrigger) {
        statTrigger = true;
        memory->request_interrupt(Interrupt::STAT_INT);
    }
    for (int i = 3; i <= 5; i++) {
        if (*stat & (1 << i) && mode == i - 3) {
            if (!statTrigger) {
                statTrigger = true;
                memory->request_interrupt(Interrupt::STAT_INT);
            }
            return;
        }
    }
    statTrigger = false;
}

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