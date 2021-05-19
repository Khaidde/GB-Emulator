#include "ppu.hpp"

#include "game_boy.hpp"

void PPU::restart() {
    lcdc = &memory->ref(IOReg::LCDC_REG);
    stat = &memory->ref(IOReg::STAT_REG);
    scy = &memory->ref(IOReg::SCY_REG);
    scx = &memory->ref(IOReg::SCX_REG);
    ly = &memory->ref(IOReg::LY_REG);
    lyc = &memory->ref(IOReg::LYC_REG);
    wy = &memory->ref(IOReg::WY_REG);
    wx = &memory->ref(IOReg::WX_REG);

    oamAddrBlock = &memory->ref(OAM_START_ADDR);

    bufferSel = false;
    for (int i = 0; i < Constants::WIDTH * Constants::HEIGHT; i++) {
        (frameBuffers[0])[i] = BLANK_COLOR;
        (frameBuffers[1])[i] = BLANK_COLOR;
    }

    drawClocks = 400;  // initial drawn clocks at PC=0x100
    mode = V_BLANK;
    *ly = 0x99;

    statTrigger = false;
    statIntrFlags = 0;
    modeSwitchClocks = 0;
}

void PPU::render(u32* pixelBuffer) {
    FrameBuffer* curBuffer = &frameBuffers[!bufferSel];
    for (int i = 0; i < Constants::WIDTH * Constants::HEIGHT; i++) {
        pixelBuffer[i] = get_lcdc_flag(LCDCFlag::LCD_ENABLE) ? (*curBuffer)[i] : BLANK_COLOR;
    }
}

extern bool test;
int cnt = 0;
void PPU::emulate_clock() {
    if (!get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
        drawClocks = 0;
        mode = Mode::H_BLANK;
        *stat = (*stat & 0xFC) | 2;
        statIntrFlags = 0;
        return;
    }
    drawClocks++;
    if (mode == Mode::OAM_SEARCH) {
        cnt = (OAM_SEARCH_CLOCKS - drawClocks) / 2;
    }
    if (mode == Mode::LCD_TRANSFER) {
        cnt = (172 - drawClocks) / 2;
    }
    if (mode == Mode::H_BLANK) {
        cnt = (LCD_AND_H_BLANK_CLOCKS - drawClocks) / 2;
    }
    if (mode == Mode::V_BLANK) {
        cnt = (SCAN_LINE_CLOCKS - drawClocks) / 2;
    }
    if (test) {
        printf("\t\t\t%d-%d-%d\n", cnt, drawClocks, statTrigger);
    }
    if (modeSwitchClocks > 0) {
        modeSwitchClocks--;
        if (modeSwitchClocks == 0) {
            *stat = (*stat & 0xFC) | mode;
            statIntrFlags &= 0x8;
            if (mode < 3 && (*stat & (1 << (mode + 3)))) {
                statIntrFlags |= 1 << mode;
                trigger_stat_intr();
            }
        }
    }
    switch (mode) {
        case Mode::OAM_SEARCH:
            if (drawClocks >= OAM_SEARCH_CLOCKS) {
                drawClocks -= OAM_SEARCH_CLOCKS;

                spriteList.clear();
                for (int i = 0; i < 40; i++) {
                    u8* spritePtr = &oamAddrBlock[i << 2];
                    u8 y = *spritePtr;
                    u8 x = *(spritePtr + 1);

                    u8 botBound = *ly + 16;
                    u8 spriteHeight = get_lcdc_flag(LCDCFlag::SPRITE_SIZE) ? 16 : 8;
                    if (x > 0) {
                        if (botBound - spriteHeight < y && y <= botBound) {
                            u8 tileID = *(spritePtr + 2);
                            u8 flags = *(spritePtr + 3);
                            spriteList.add({y, x, tileID, flags});
                            if (spriteList.size == SpriteList::MAX_SPRITES) break;
                        }
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
            }
            break;
        case LCD_TRANSFER: {
            if (pxlFifo.size > 8 && !fetcher.curSprite) {
                handle_pixel_push();
            }

            if (fetcher.doFetch) {
                if (fetcher.curSprite) {
                    sprite_fetch();
                } else {
                    background_fetch();
                }
            }
            fetcher.doFetch = !fetcher.doFetch;

            if (curPixelX == Constants::WIDTH) {
                // printf("----------------------ly=%d,c=%d\n", *ly, drawClocks);
                set_mode(H_BLANK);
            }
        } break;
        case H_BLANK:
            if (drawClocks >= LCD_AND_H_BLANK_CLOCKS) {
                drawClocks -= LCD_AND_H_BLANK_CLOCKS;
                (*ly)++;

                if (*ly >= Constants::HEIGHT) {
                    memory->request_interrupt(Interrupt::VBLANK_INT);
                    set_mode(V_BLANK);
                } else {
                    set_mode(OAM_SEARCH);
                }
                update_coincidence();
                trigger_stat_intr();
            }
            break;
        case V_BLANK:
            if (drawClocks == 5 && *ly == V_BLANK_END_LINE - 1) {
                update_coincidence();
                trigger_stat_intr();
            }
            if (drawClocks >= SCAN_LINE_CLOCKS) {
                drawClocks -= SCAN_LINE_CLOCKS;
                (*ly)++;

                if (*ly >= V_BLANK_END_LINE) {
                    bufferSel = !bufferSel;
                    *ly = 0;
                    set_mode(OAM_SEARCH);
                }
                update_coincidence();
                trigger_stat_intr();
            }
            break;
    }
}

void PPU::update_coincidence() {
    bool coincidence = read_ly() == *lyc;
    *stat = (*stat & 0xFB) | (coincidence << 2);

    bool lyLycEn = *stat & (1 << 6);
    statIntrFlags = ((lyLycEn && coincidence) << 3) | (statIntrFlags & 0x7);
}

void PPU::update_stat() {
    statIntrFlags &= 0x8;
    for (int i = 3; i <= 5; i++) {
        if (*stat & (1 << i) && (*stat & 0x3) == i - 3) {
            statIntrFlags |= 1 << (i - 3);
        }
    }
    update_coincidence();
}

void PPU::trigger_stat_intr() {
    if (statIntrFlags) {
        if (!statTrigger) {
            statTrigger = true;
            memory->request_interrupt(Interrupt::STAT_INT);
        }
    } else {
        statTrigger = false;
    }
}

u8 PPU::read_ly() {
    if (*ly == 0x99 && drawClocks > 4) return 0;
    return *ly;
}

bool PPU::is_vram_blocked() { return false; }
bool PPU::is_oam_blocked() { return *stat & 0x2; }

void PPU::handle_pixel_push() {
    if (!fetcher.windowMode && get_lcdc_flag(LCDCFlag::WINDOW_ENABLE) && *ly >= *wy && curPixelX >= *wx - 7) {
        fetcher.tileX = 0;
        fetcher.windowMode = true;
        pxlFifo.clear();
        return;
    }
    if (!fetcher.windowMode && numDiscardedPixels < *scx % TILE_PX_SIZE) {
        numDiscardedPixels++;
        pxlFifo.pop_head();
        return;
    }
    if (get_lcdc_flag(LCDCFlag::SPRITE_ENABLE)) {
        for (int i = 0; i < spriteList.size; i++) {
            if ((spriteList.removedBitField & (1 << i)) != 0) continue;

            Sprite* sprite = &spriteList.data[i];
            if (sprite->x <= curPixelX + 8) {
                fetcher.doFetch = false;
                fetcher.fetchState = Fetcher::READ_TILE_ID;
                fetcher.curSprite = sprite;
                spriteList.remove(i);
                break;
            }
        }
    }
    if (!fetcher.curSprite) {
        frameBuffers[bufferSel][*ly * Constants::WIDTH + curPixelX++] = get_color(pxlFifo.pop_head());
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
            u8 tileByteOff;
            if (fetcher.windowMode) {
                u8 tileX = fetcher.tileX % TILESET_SIZE;
                u8 tileY = ((*ly - *wy) / TILE_PX_SIZE) % TILESET_SIZE;
                tileIndex = memory->read((tileX + tileY * TILESET_SIZE) + tileMap);
                tileByteOff = ((*ly - *wy) % TILE_PX_SIZE) * 2;
            } else {
                u8 tileX = (*scx / TILE_PX_SIZE + fetcher.tileX) % TILESET_SIZE;
                u8 tileY = ((*scy + *ly) / TILE_PX_SIZE) % TILESET_SIZE;
                tileIndex = memory->read((tileX + tileY * TILESET_SIZE) + tileMap);
                tileByteOff = ((*scy + *ly) % TILE_PX_SIZE) * 2;
            }

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
                        pxlFifo.push_tail({col, false, IOReg::BGP_REG});
                    }
                } else {
                    for (int i = 0; i < TILE_PX_SIZE; i++) {
                        pxlFifo.push_tail({0, false, IOReg::BGP_REG});
                    }
                }
                fetcher.tileX++;
                fetcher.fetchState = Fetcher::READ_TILE_ID;
            }
            break;
        default:
            break;
    }
}

void PPU::sprite_fetch() {
    switch (fetcher.fetchState) {
        case Fetcher::READ_TILE_ID: {
            u8 tileIndex = fetcher.curSprite->tileID;
            if (get_lcdc_flag(LCDCFlag::SPRITE_SIZE)) {
                tileIndex &= ~0x1;  // Make tileIndex even
            }

            u8 tileByteOff;
            bool yFlip = fetcher.curSprite->flags & (1 << 6);
            if (yFlip) {
                tileByteOff = (fetcher.curSprite->y - *ly - 0x1) * 2;
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

void PPU::set_mode(Mode m) {
    // When switching from oam_search to v_blank, momentarily switch to h_blank
    if (m == OAM_SEARCH && this->mode == V_BLANK) {
        *stat = (*stat & 0xFC);
    }
    // TODO only delay stat mode change on DMG
    this->modeSwitchClocks = 4;
    this->mode = m;
}

u32 PPU::get_color(FIFOData&& data) {
    data.colIndex &= 0x03;

    u8 i = (data.colIndex << 1);
    u8 mask = 3 << i;

    return baseColors[(memory->read(data.palleteAddr) & mask) >> i];
}
