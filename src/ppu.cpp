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

    line = 0x99;
    curPPUState = PPUState::V_BLANK;
    clockCnt = 56;  // initial drawn clocks at PC=0x100

    statIntrFlags = 0;
    prevIntrFlags = 0;
    internalStatEnable = 0;
}

void PPU::write_register(u16 addr, u8 val) {
    switch (addr) {
        case IOReg::LCDC_REG: {
            bool lcdOn = (val & (1 << 7));
            if (lcdOn && !get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
                curPPUState = PPUState::OAM;
                clockCnt = OAM_SEARCH_CLOCKS - 4;
                ppuClocks = 4;  // Hack to enable correct lcd on timing
                try_lyc_intr();
                try_trigger_stat();
            } else if (!lcdOn && get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
                curPPUState = PPUState::OAM_3;
                clockCnt = 3;
                *stat = (*stat & 0xFC) | 0;
                *ly = 0;
                update_coincidence();
                line = 0;
                statIntrFlags = 0;
                try_lyc_intr();
                try_trigger_stat();
            }
            *lcdc = val;
        } break;
        case IOReg::STAT_REG: {
            u8 changedEnable = ((*stat ^ val) >> 3) & internalStatEnable;
            *stat = 0x80 | (val & 0x78) | (*stat & 0x7);
            for (int i = 0; i <= 2; i++) {
                if (changedEnable & (1 << i)) {
                    try_mode_intr(i);
                }
            }
            try_lyc_intr();
            try_trigger_stat();
        } break;
        case IOReg::LYC_REG:
            *lyc = val;
            update_coincidence();
            try_lyc_intr();
            try_trigger_stat();
            break;
    }
}

bool PPU::is_vram_blocked() { return (*stat & 0x3) == 0x3; }

bool PPU::is_oam_blocked() { return *stat & 0x2; }

void PPU::render(u32* pixelBuffer) {
    FrameBuffer* curBuffer = &frameBuffers[!bufferSel];
    for (int i = 0; i < Constants::WIDTH * Constants::HEIGHT; i++) {
        pixelBuffer[i] = get_lcdc_flag(LCDCFlag::LCD_ENABLE) ? (*curBuffer)[i] : BLANK_COLOR;
    }
}

void PPU::emulate_clock() {
    if (!get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
        return;
    }
    ppuClocks++;
    debugger->log_ppu((int)curPPUState, clockCnt / 2);
    if (clockCnt-- > 1) {
        return;
    }
    switch (curPPUState) {
        case PPUState::OAM_3:
            if (line == 0) {
                // When switching from v_blank to oam_search, momentarily switch to h_blank
                // Timing is unverified
                set_stat_mode(H_BLANK_MODE);
            } else {
                // OAM interrupt occurs 1 T-cycle before stat mode change when line > 0
                try_mode_intr(OAM_MODE);
                internalStatEnable |= 1 << 2;
                try_trigger_stat();
                clear_stat_mode_intr(H_BLANK_MODE);
            }
            curPPUState = PPUState::OAM_4;
            break;
        case PPUState::OAM_4:
            set_stat_mode(OAM_MODE);
            if (line == 0) {
                try_mode_intr(OAM_MODE);
                internalStatEnable |= 1 << 2;
                try_trigger_stat();
                clear_stat_mode_intr(V_BLANK_MODE);
            } else {
                try_lyc_intr();
            }
            try_trigger_stat();

            curPPUState = PPUState::OAM;
            clockCnt = OAM_SEARCH_CLOCKS - 4;
            break;
        case PPUState::OAM:
            spriteList.clear();
            for (int i = 0; i < 40; i++) {
                u8* spritePtr = &oamAddrBlock[i << 2];
                u8 y = *spritePtr;
                u8 x = *(spritePtr + 1);

                u8 botBound = line + 16;
                u8 spriteHeight = get_lcdc_flag(LCDCFlag::SPRITE_SIZE) ? 16 : 8;
                if (x > 0 && botBound - spriteHeight < y && y <= botBound) {
                    u8 tileID = *(spritePtr + 2);
                    u8 flags = *(spritePtr + 3);
                    spriteList.add({y, x, tileID, flags});
                    if (spriteList.size == SpriteList::MAX_SPRITES) break;
                }
            }

            fetcher.reset();
            lockedSCX = *scx;
            numDiscardedPixels = 0;
            curPixelX = 0;
            pxlFifo.clear();

            curPPUState = PPUState::LCD_4;
            ppuClocks = 0;
            break;
        case PPUState::LCD_4:
            if (ppuClocks == 4) {
                set_stat_mode(LCD_MODE);
                clear_stat_mode_intr(OAM_MODE);
                curPPUState = PPUState::LCD;
            }
        case PPUState::LCD:
            clockCnt = 0;
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
                curPPUState = PPUState::H_BLANK_4;
                clockCnt = 4;
            }
            break;
        case PPUState::H_BLANK_4:
            set_stat_mode(H_BLANK_MODE);
            try_mode_intr(H_BLANK_MODE);
            internalStatEnable |= 1;
            try_trigger_stat();

            curPPUState = PPUState::H_BLANK;
            clockCnt = LCD_AND_H_BLANK_CLOCKS - ppuClocks;
            break;
        case PPUState::H_BLANK:
            (*ly)++;
            update_coincidence();

            line++;
            if (line >= Constants::HEIGHT) {
                curPPUState = PPUState::V_BLANK_144_4;
                clockCnt = 4;
                bufferSel = !bufferSel;
            } else {
                curPPUState = PPUState::OAM_3;
                clockCnt = 3;
            }
            break;
        case PPUState::V_BLANK_144_4:
            set_stat_mode(V_BLANK_MODE);

            memory->request_interrupt(Interrupt::VBLANK_INT);
            try_lyc_intr();
            try_mode_intr(V_BLANK_MODE);
            internalStatEnable |= 1 << 1;

            // OAM interrupt can occur on line 144
            try_mode_intr(OAM_MODE);

            try_trigger_stat();
            clear_stat_mode_intr(OAM_MODE);
            clear_stat_mode_intr(H_BLANK_MODE);

            curPPUState = PPUState::V_BLANK;
            clockCnt = SCAN_LINE_CLOCKS - 4;
            break;
        case PPUState::V_BLANK_145_152_4:
            try_lyc_intr();
            try_trigger_stat();

            curPPUState = PPUState::V_BLANK;
            clockCnt = SCAN_LINE_CLOCKS - 4;
            break;
        case PPUState::V_BLANK_153_4:
            *ly = 0;
            update_coincidence();
            try_lyc_intr();
            try_trigger_stat();

            curPPUState = PPUState::V_BLANK_153_12;
            clockCnt = 8;
            break;
        case PPUState::V_BLANK_153_12:
            try_lyc_intr();
            try_trigger_stat();

            curPPUState = PPUState::V_BLANK;
            clockCnt = SCAN_LINE_CLOCKS - 12;
            break;
        case PPUState::V_BLANK:
            line++;
            if (line < V_BLANK_END_LINE - 1) {
                (*ly)++;
                update_coincidence();

                curPPUState = PPUState::V_BLANK_145_152_4;
                clockCnt = 4;
            } else if (line == V_BLANK_END_LINE - 1) {
                curPPUState = PPUState::V_BLANK_153_4;
                clockCnt = 4;
            } else {
                line = 0;

                curPPUState = PPUState::OAM_3;
                clockCnt = 3;
            }
            break;
    }
}

void PPU::set_stat_mode(PPUMode statMode) { *stat = (*stat & ~0x3) | statMode; }

void PPU::update_coincidence() { *stat = (*stat & ~0x4) | ((*ly == *lyc) << 2); }

void PPU::try_lyc_intr() {
    bool coincidence = *ly == *lyc;
    bool lyLycEn = *stat & (1 << 6);
    statIntrFlags &= 0x7;
    statIntrFlags |= (lyLycEn && coincidence) << 3;
}

void PPU::try_mode_intr(u8 statMode) {
    if (*stat & (1 << (statMode + 3))) {
        statIntrFlags |= 1 << statMode;
    } else {
        statIntrFlags &= ~(1 << statMode);
    }
}

void PPU::clear_stat_mode_intr(PPUMode statMode) {
    internalStatEnable &= ~(1 << statMode);
    statIntrFlags &= ~(1 << statMode);
}

void PPU::try_trigger_stat() {
    if (!prevIntrFlags && statIntrFlags) {
        memory->request_interrupt(Interrupt::STAT_INT);
    }
    prevIntrFlags = statIntrFlags;
}

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
