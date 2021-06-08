#include "ppu.hpp"

#include "game_boy.hpp"

void SpriteList::clear() {
    size = 0;
    removedBitField = 0;
}

void SpriteList::add(Sprite&& sprite) { data[size++] = sprite; }

void SpriteList::remove(u8 index) { removedBitField |= 1 << index; }

void Fetcher::reset() {
    doFetch = false;
    fetchState = Fetcher::READ_TILE_ID;
    curSprite = nullptr;
    windowMode = false;
    tileX = 0;
}

PPU::PPU(Memory* memory) : memory(memory) {
    lcdc = &memory->ref(IOReg::LCDC_REG);
    stat = &memory->ref(IOReg::STAT_REG);
    scy = &memory->ref(IOReg::SCY_REG);
    scx = &memory->ref(IOReg::SCX_REG);
    ly = &memory->ref(IOReg::LY_REG);
    lyc = &memory->ref(IOReg::LYC_REG);
    wy = &memory->ref(IOReg::WY_REG);
    wx = &memory->ref(IOReg::WX_REG);

    oamAddrBlock = &memory->ref(OAM_START_ADDR);
    vramAddrBlock = &memory->ref(VRAM_START_ADDR);
}

void PPU::restart() {
    bufferSel = 0;
    for (int i = 0; i < Constants::WIDTH * Constants::HEIGHT; i++) {
        (frameBuffers[0])[i] = BLANK_COLOR;
        (frameBuffers[1])[i] = BLANK_COLOR;
    }

    drawClocks = 400;  // initial drawn clocks at PC=0x100
    curPPUState = PPUState::V_BLANK;
    line = 0x99;

    statIntrFlags = 0;
    internalStatEnable = 0;
    lcdLineCycles = 0;
}

void PPU::render(u32* pixelBuffer) {
    FrameBuffer* curBuffer = &frameBuffers[!bufferSel];
    for (int i = 0; i < Constants::WIDTH * Constants::HEIGHT; i++) {
        pixelBuffer[i] = get_lcdc_flag(LCDCFlag::LCD_ENABLE) ? (*curBuffer)[i] : BLANK_COLOR;
    }
}

extern bool test;
int cnt = 0;
static int spriteCount = 0;
void PPU::emulate_clock() {
    if (!get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
        return;
    }
    drawClocks++;
    /*
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
    */
    if (test) {
        printf("\t\t\t%d-%d-%02x\n", cnt, drawClocks, statIntrFlags);
    }
    switch (curPPUState) {
        case PPUState::OAM_0:
            if (drawClocks == 3) {
                if (line == 0) {
                    // When switching from v_blank to oam_search, momentarily switch to h_blank
                    // Timing is unverified
                    set_stat_mode(H_BLANK_MODE);
                } else {
                    // OAM interrupt occurs 1 T-cycle before stat mode change when line > 0
                    update_mode_intr(OAM_MODE);
                    clear_stat_mode_intr(H_BLANK_MODE);
                }
                curPPUState = PPUState::OAM_3;
            }
            break;
        case PPUState::OAM_3:
            set_stat_mode(OAM_MODE);
            if (line == 0) {
                update_mode_intr(OAM_MODE);
                clear_stat_mode_intr(V_BLANK_MODE);
            } else {
                try_lyc_intr();
            }
            curPPUState = PPUState::OAM_4;
            break;
        case PPUState::OAM_4:
            if (drawClocks == OAM_SEARCH_CLOCKS) {
                drawClocks = 0;
                spriteList.clear();
                for (int i = 0; i < 40; i++) {
                    u8* spritePtr = &oamAddrBlock[i << 2];
                    u8 y = *spritePtr;
                    u8 x = *(spritePtr + 1);

                    u8 botBound = line + 16;
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
                spriteCount = spriteList.size;

                fetcher.reset();

                lockedSCX = *scx;

                numDiscardedPixels = 0;
                curPixelX = 0;

                pxlFifo.clear();

                curPPUState = PPUState::LCD_0;
            }
            break;
        case PPUState::LCD_0:
            if (drawClocks == 4) {
                set_stat_mode(LCD_MODE);
                clear_stat_mode_intr(OAM_MODE);
                curPPUState = PPUState::LCD_4;
            }
        case PPUState::LCD_4:
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
                lcdLineCycles = drawClocks;
                curPPUState = PPUState::H_BLANK_0;
            }
            break;
        case PPUState::H_BLANK_0:
            if (drawClocks - lcdLineCycles == 4) {
                set_stat_mode(H_BLANK_MODE);
                update_mode_intr(H_BLANK_MODE);
                curPPUState = PPUState::H_BLANK_4;
            }
            break;
        case PPUState::H_BLANK_4:
            if (drawClocks == LCD_AND_H_BLANK_CLOCKS) {
                drawClocks = 0;
                (*ly)++;
                line++;
                if (line >= Constants::HEIGHT) {
                    curPPUState = PPUState::V_BLANK_144_0;
                    bufferSel = !bufferSel;
                } else {
                    curPPUState = PPUState::OAM_0;
                }
            }
            break;
        case PPUState::V_BLANK_144_0:
            if (drawClocks == 4) {
                memory->request_interrupt(Interrupt::VBLANK_INT);
                update_mode_intr(V_BLANK_MODE);

                // OAM interrupt can occur on line 144
                update_mode_intr(OAM_MODE);
                clear_stat_mode_intr(OAM_MODE);

                clear_stat_mode_intr(H_BLANK_MODE);

                try_lyc_intr();
                curPPUState = PPUState::V_BLANK;
            }
            break;
        case PPUState::V_BLANK_145_152_0:
            if (drawClocks == 4) {
                try_lyc_intr();
                curPPUState = PPUState::V_BLANK;
            }
            break;
        case PPUState::V_BLANK_153_0:
            if (drawClocks == 4) {
                *ly = 0;
                try_lyc_intr();
                curPPUState = PPUState::V_BLANK_153_4;
            }
            break;
        case PPUState::V_BLANK_153_4:
            if (drawClocks == 12) {
                try_lyc_intr();
                curPPUState = PPUState::V_BLANK;
            }
            break;
        case PPUState::V_BLANK:
            if (drawClocks == SCAN_LINE_CLOCKS) {
                drawClocks = 0;
                line++;
                if (line < V_BLANK_END_LINE - 1) {
                    (*ly)++;
                    curPPUState = PPUState::V_BLANK_145_152_0;
                } else if (line == V_BLANK_END_LINE - 1) {
                    curPPUState = PPUState::V_BLANK_153_0;
                } else {
                    line = 0;
                    curPPUState = PPUState::OAM_0;
                }
            }
            break;
    }
}

void PPU::write_register(u16 addr, u8 val) {
    switch (addr) {
        case IOReg::LCDC_REG: {
            bool lcdOn = (val & (1 << 7));
            if (lcdOn && !get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
                drawClocks = 4;  // Hack to enable correct lcd on timing
                try_lyc_intr();
            } else if (!lcdOn && get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
                curPPUState = PPUState::OAM_0;
                *stat = (*stat & 0xFC) | 0;
                *ly = 0;
                line = 0;
                statIntrFlags = 0;
                try_lyc_intr();
            }
            *lcdc = val;
        } break;
        case IOReg::STAT_REG: {
            u8 changedEnable = ((*stat ^ val) >> 3) & internalStatEnable;
            *stat = 0x80 | (val & 0x78) | (*stat & 0x7);
            for (int i = 0; i <= 2; i++) {
                if (changedEnable & (1 << i)) {
                    printf("%d-%d-%02x\n", i, (int)curPPUState, line);
                    try_mode_intr(i);
                }
            }
            try_lyc_intr();
        } break;
        case IOReg::LYC_REG:
            *lyc = val;
            try_lyc_intr();
            break;
    }
}

void PPU::set_stat_mode(PPUMode statMode) { *stat = (*stat & ~0x3) | statMode; }

void PPU::try_lyc_intr() {
    u8 prevIntrFlags = statIntrFlags;

    bool coincidence = *ly == *lyc;
    bool lyLycEn = *stat & (1 << 6);
    statIntrFlags &= 0x7;
    statIntrFlags |= (lyLycEn && coincidence) << 3;

    if (!prevIntrFlags && statIntrFlags) {
        memory->request_interrupt(Interrupt::STAT_INT);
    }
}

void PPU::try_mode_intr(u8 statMode) {
    u8 prevIntrFlags = statIntrFlags;
    if (*stat & (1 << (statMode + 3))) {
        statIntrFlags |= 1 << statMode;
        if (!prevIntrFlags) {
            memory->request_interrupt(Interrupt::STAT_INT);
        }
    } else {
        statIntrFlags &= ~(1 << statMode);
    }
}

void PPU::update_mode_intr(PPUMode statMode) {
    try_mode_intr(statMode);
    internalStatEnable |= 1 << statMode;
}

void PPU::clear_stat_mode_intr(PPUMode statMode) {
    internalStatEnable &= ~(1 << statMode);
    statIntrFlags &= ~(1 << statMode);
}

bool PPU::is_vram_blocked() { return (*stat & 0x3) == 0x3; }
bool PPU::is_oam_blocked() { return *stat & 0x2; }

void PPU::handle_pixel_push() {
    if (!fetcher.windowMode && get_lcdc_flag(LCDCFlag::WINDOW_ENABLE) && line >= *wy && curPixelX >= *wx - 7) {
        fetcher.doFetch = false;
        fetcher.fetchState = Fetcher::READ_TILE_ID;

        fetcher.tileX = 0;
        fetcher.windowMode = true;
        pxlFifo.clear();
        return;
    }
    if (!fetcher.windowMode && numDiscardedPixels < lockedSCX % TILE_PX_SIZE) {
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
        frameBuffers[bufferSel][line * Constants::WIDTH + curPixelX++] = get_color(pxlFifo.pop_head());
    }
}

void PPU::background_fetch() {
    switch (fetcher.fetchState) {
        case Fetcher::READ_TILE_ID: {
            u16 tileMap = (fetcher.windowMode ? get_lcdc_flag(LCDCFlag::WINDOW_TILE_SELECT)
                                              : get_lcdc_flag(LCDCFlag::BG_TILE_SELECT))
                              ? 0x9C00
                              : 0x9800;

            u16 xOff = fetcher.windowMode ? 0 : lockedSCX;
            u16 yOff = fetcher.windowMode ? (line - *wy) : (*scy + line);

            u8 tileX = (xOff / TILE_PX_SIZE + fetcher.tileX) % TILESET_SIZE;
            u8 tileY = (yOff / TILE_PX_SIZE) % TILESET_SIZE;
            s8 tileIndex = vramAddrBlock[(tileX + tileY * TILESET_SIZE) + tileMap - VRAM_START_ADDR];
            u8 tileByteOff = (yOff % TILE_PX_SIZE) * 2;

            if (get_lcdc_flag(LCDCFlag::TILE_DATA_SELECT)) {
                fetcher.tileRowAddrOff = (u8)tileIndex * TILE_MEM_LEN + tileByteOff;
            } else {
                fetcher.tileRowAddrOff = VRAM_TILE_DATA_1 - VRAM_TILE_DATA_0 + tileIndex * TILE_MEM_LEN + tileByteOff;
            }
            fetcher.fetchState = Fetcher::READ_TILE_0;
        } break;
        case Fetcher::READ_TILE_0:
            fetcher.data0 = vramAddrBlock[fetcher.tileRowAddrOff];
            fetcher.fetchState = Fetcher::READ_TILE_1;
            break;
        case Fetcher::READ_TILE_1:
            fetcher.data1 = vramAddrBlock[fetcher.tileRowAddrOff + 1];
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
                tileByteOff = (fetcher.curSprite->y - line - 0x1) * 2;
            } else {
                tileByteOff = (0x10 - (fetcher.curSprite->y - line)) * 2;
            }

            fetcher.tileRowAddrOff = tileIndex * TILE_MEM_LEN + tileByteOff;
            fetcher.fetchState = Fetcher::READ_TILE_0;
        } break;
        case Fetcher::READ_TILE_0:
            fetcher.data0 = vramAddrBlock[fetcher.tileRowAddrOff];
            fetcher.fetchState = Fetcher::READ_TILE_1;
            break;
        case Fetcher::READ_TILE_1:
            fetcher.data1 = vramAddrBlock[fetcher.tileRowAddrOff + 1];
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

u32 PPU::get_color(FIFOData&& data) {
    data.colIndex &= 0x03;

    u8 i = (data.colIndex << 1);
    u8 mask = 3 << i;

    return baseColors[(memory->read(data.palleteAddr) & mask) >> i];
}