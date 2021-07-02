#include "ppu.hpp"

#include "game_boy.hpp"

void SpriteList::clear() {
    size = 0;
    removedBitField = 0;
}

void SpriteList::add(Sprite&& sprite) { data[size++] = sprite; }

void SpriteList::remove(u8 index) { removedBitField |= 1 << index; }

PPU::PPU(Memory& memory) : memory(&memory) {
    lcdc = &memory.ref(IOReg::LCDC_REG);
    stat = &memory.ref(IOReg::STAT_REG);
    scy = &memory.ref(IOReg::SCY_REG);
    scx = &memory.ref(IOReg::SCX_REG);
    ly = &memory.ref(IOReg::LY_REG);
    lyc = &memory.ref(IOReg::LYC_REG);
    wy = &memory.ref(IOReg::WY_REG);
    wx = &memory.ref(IOReg::WX_REG);

    oamAddrBlock = &memory.ref(OAM_START_ADDR);
    vramAddrBlock = &memory.ref(VRAM_START_ADDR);
}

void PPU::restart() {
    bufferSel = 0;
    for (int i = 0; i < Constants::WIDTH * Constants::HEIGHT; i++) {
        (frameBuffers[0])[i] = BLANK_COLOR;
        (frameBuffers[1])[i] = BLANK_COLOR;
    }

    line = 0x99;
    curPPUState = PPUState::V_BLANK;
    clockCnt = 56;  // initial drawn clocks at PC=0x100 (DMG)
    lineClocks = 400;

    oamBlockRead = false;
    oamBlockWrite = false;
    vramBlockRead = false;
    vramBlockWrite = false;

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
                // TODO for some reason, turning lcd on using ldh (ff00) causes ldh to take effect
                // immediately
                lineClocks = 4;  // Hack to enable correct lcd on timing
                clockCnt = OAM_SEARCH_CLOCKS - 4;
                update_coincidence();
                try_lyc_intr();
                try_trigger_stat();
            } else if (!lcdOn && get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
                *stat = (*stat & 0xFC) | 0;
                *ly = 0;
                line = 0;
                update_coincidence();
                statIntrFlags = 0;
                try_lyc_intr();
                try_trigger_stat();

                oamBlockRead = false;
                oamBlockWrite = false;
                vramBlockRead = false;
                vramBlockWrite = false;
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

bool PPU::is_oam_read_blocked() { return oamBlockRead; }

bool PPU::is_oam_write_blocked() { return oamBlockWrite; }

bool PPU::is_vram_read_blocked() { return vramBlockRead; }

bool PPU::is_vram_write_blocked() { return vramBlockWrite; }

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
    lineClocks++;
    if (clockCnt > 1) {
        clockCnt--;
        return;
    }
    switch (curPPUState) {
        case PPUState::OAM_3:
            if (line == 0) {
                fetcher.windowMode = false;
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
            update_coincidence();
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

            oamBlockWrite = true;
            curPPUState = PPUState::OAM_PRE_1;
            clockCnt = OAM_SEARCH_CLOCKS - 4 - 1;
            break;
        case PPUState::OAM_PRE_1:
            vramBlockRead = true;
            curPPUState = PPUState::OAM;
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

            fetcher.curFetchState = 0;
            fetcher.curSprite = nullptr;
            fetcher.windowXEnable = false;
            fetcher.tileX = 0;

            lockedSCX = *scx;
            curPixelX = -8 - (lockedSCX & 0x7);  // Fifo starts pushing pixels offscreen
            bgFifo.clear();
            spriteFifo.clear();

            oamBlockWrite = false;  // OAM is momentarily accessible after oam mode
            curPPUState = PPUState::LCD_4;
            clockCnt = 4;
            break;
        case PPUState::LCD_4:
            set_stat_mode(LCD_MODE);
            clear_stat_mode_intr(OAM_MODE);

            oamBlockRead = true;
            oamBlockWrite = true;
            vramBlockWrite = true;
            vramBlockRead = true;
            curPPUState = PPUState::LCD_5;
            clockCnt = 1;
            break;
        case PPUState::LCD_5:
            // Fetcher firt grabs tile 1 in 6 cycles but trashes it. Since the tile
            // is trashed, we simplify this by filling the fifo with garbage values.
            for (int i = 0; i < 8; i++) {
                bgFifo.push_tail({0, DMGPalette::BGP, false});
            }
            curPPUState = PPUState::LCD;
            break;
        case PPUState::LCD:
            handle_pixel_render();

            if (fetcher.curSprite) {
                sprite_fetch();
            } else {
                background_fetch();
            }
            fetcher.curFetchState = (fetcher.curFetchState + 1) & 0x7;

            if (curPixelX == Constants::WIDTH - 1) {
                handle_pixel_render();
                if (fetcher.curSprite) {
                    sprite_fetch();
                } else {
                    background_fetch();
                }

                fetcher.windowXEnable = false;

                curPPUState = PPUState::H_BLANK_4;
                if (line == 130) {
                    // printf("cc=%d-%d\n", lineClocks - 80, *scx);
                }
                clockCnt = 4;
            }
            break;
        case PPUState::H_BLANK_4:
            set_stat_mode(H_BLANK_MODE);
            try_mode_intr(H_BLANK_MODE);
            internalStatEnable |= 1;
            try_trigger_stat();

            oamBlockRead = false;
            oamBlockWrite = false;
            vramBlockRead = false;
            vramBlockWrite = false;

            curPPUState = PPUState::H_BLANK;
            clockCnt = SCAN_LINE_CLOCKS - lineClocks;
            break;
        case PPUState::H_BLANK:
            (*ly)++;
            clear_coincidence();  // Coincidence is always 0 here when line > 0

            line++;
            lineClocks = 0;
            if (line >= Constants::HEIGHT) {
                curPPUState = PPUState::V_BLANK_144_4;
                clockCnt = 4;
                bufferSel = !bufferSel;
            } else {
                oamBlockRead = true;
                curPPUState = PPUState::OAM_3;
                clockCnt = 3;
            }
            break;
        case PPUState::V_BLANK_144_4:
            update_coincidence();
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
            clockCnt = SCAN_LINE_CLOCKS - lineClocks;
            break;
        case PPUState::V_BLANK_145_152_4:
            update_coincidence();
            try_lyc_intr();
            try_trigger_stat();

            curPPUState = PPUState::V_BLANK;
            clockCnt = SCAN_LINE_CLOCKS - lineClocks;
            break;
        case PPUState::V_BLANK_153_4:
            update_coincidence();
            *ly = 0;
            try_lyc_intr();
            try_trigger_stat();

            curPPUState = PPUState::V_BLANK_153_8;
            clockCnt = 4;
            break;
        case PPUState::V_BLANK_153_8:
            clear_coincidence();

            curPPUState = PPUState::V_BLANK_153_12;
            clockCnt = 4;
            break;
        case PPUState::V_BLANK_153_12:
            update_coincidence();
            try_lyc_intr();
            try_trigger_stat();

            curPPUState = PPUState::V_BLANK;
            clockCnt = SCAN_LINE_CLOCKS - lineClocks;
            break;
        case PPUState::V_BLANK:
            line++;
            lineClocks = 0;
            if (line < V_BLANK_END_LINE - 1) {
                (*ly)++;
                clear_coincidence();  // Coincidence is always 0 here

                curPPUState = PPUState::V_BLANK_145_152_4;
                clockCnt = 4;
            } else if (line == V_BLANK_END_LINE - 1) {
                curPPUState = PPUState::V_BLANK_153_4;
                clockCnt = 4;
            } else {
                line = 0;

                oamBlockRead = true;
                curPPUState = PPUState::OAM_3;
                clockCnt = 3;
            }
            break;
    }
}

void PPU::set_stat_mode(PPUMode statMode) { *stat = (*stat & ~0x3) | statMode; }

void PPU::update_coincidence() { *stat = (*stat & ~0x4) | ((*ly == *lyc) << 2); }

void PPU::clear_coincidence() { *stat &= ~0x4; }

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

void PPU::handle_pixel_render() {
    if (fetcher.curSprite) {
        return;
    }
    if (line == *wy) {
        fetcher.windowMode = true;
        fetcher.windowY = 0;
    }
    if (fetcher.windowMode && !fetcher.windowXEnable && curPixelX == *wx - 7) {
        if (get_lcdc_flag(LCDCFlag::WINDOW_ENABLE)) {
            fetcher.curFetchState = 0;

            fetcher.tileX = 0;
            fetcher.windowXEnable = true;
            bgFifo.clear();

            if (line != *wy) {
                fetcher.windowY++;
            }
            return;
        }
    }
    if (get_lcdc_flag(LCDCFlag::SPRITE_ENABLE)) {
        for (int i = 0; i < spriteList.size; i++) {
            if ((spriteList.removedBitField & (1 << i)) != 0) continue;

            Sprite* sprite = &spriteList.data[i];
            if (sprite->x <= curPixelX + 8) {
                fetcher.curFetchState = 0;
                fetcher.curSprite = sprite;
                spriteList.remove(i);
                return;
            }
        }
    }
    if (bgFifo.size > 0) {
        FIFOData bgPxl = bgFifo.pop_head();
        u8 colIndex = bgPxl.colIndex;
        u8 paletteNum = bgPxl.paletteNum;
        if (spriteFifo.size > 0) {
            FIFOData spritePxl = spriteFifo.pop_head();
            if (spritePxl.colIndex != 0) {
                if (!spritePxl.bgPriority || colIndex == 0) {
                    colIndex = spritePxl.colIndex;
                    paletteNum = spritePxl.paletteNum;
                }
            }
        }

        if (curPixelX >= 0) {
            u8 i = (colIndex & 0x3) << 1;
            u8 mask = 3 << i;
            u8 palette = memory->read(IOReg::BGP_REG + paletteNum);
            u32 col = baseColors[(palette & mask) >> i];
            frameBuffers[bufferSel][line * Constants::WIDTH + curPixelX] = col;
        }
        if (line == 130) {
            // printf("---%d\n", curPixelX);
        }
        curPixelX++;
    }
}

void PPU::background_fetch() {
    switch (fetcher.FETCH_STATES[fetcher.curFetchState]) {
        case Fetcher::READ_TILE_ID: {
            // TODO verify that flag is checked and turns off during READ_TILE_ID
            if (fetcher.windowXEnable && !get_lcdc_flag(LCDCFlag::WINDOW_ENABLE)) {
                fetcher.windowXEnable = false;
            }

            u16 tileMap = (fetcher.windowXEnable ? get_lcdc_flag(LCDCFlag::WINDOW_TILE_SELECT)
                                                 : get_lcdc_flag(LCDCFlag::BG_TILE_SELECT))
                              ? 0x9C00
                              : 0x9800;

            u16 xOff = fetcher.windowXEnable ? 0 : lockedSCX;
            u16 yOff = fetcher.windowXEnable ? fetcher.windowY : (*scy + line);

            u8 tileX = (xOff / TILE_PX_SIZE + fetcher.tileX) % TILESET_SIZE;
            u8 tileY = (yOff / TILE_PX_SIZE) % TILESET_SIZE;
            s8 tileIndex =
                (s8)vramAddrBlock[(tileX + tileY * TILESET_SIZE) + tileMap - VRAM_START_ADDR];
            u8 tileByteOff = (yOff % TILE_PX_SIZE) * 2;

            if (get_lcdc_flag(LCDCFlag::TILE_DATA_SELECT)) {
                fetcher.tileRowAddrOff = (u8)tileIndex * TILE_MEM_LEN + tileByteOff;
            } else {
                fetcher.tileRowAddrOff = (u16)(VRAM_TILE_DATA_1 - VRAM_TILE_DATA_0 +
                                               tileIndex * TILE_MEM_LEN + tileByteOff);
            }
            if (line == 130) {
                // printf("b-%d\n", ppuClocks);
            }
        } break;
        case Fetcher::READ_TILE_0:
            fetcher.data0 = vramAddrBlock[fetcher.tileRowAddrOff];
            if (line == 130) {
                // printf("0-%d\n", ppuClocks);
            }
            break;
        case Fetcher::READ_TILE_1:
            fetcher.data1 = vramAddrBlock[fetcher.tileRowAddrOff + 1];
            if (line == 130) {
                // printf("1-%d\n", ppuClocks);
            }
        case Fetcher::PUSH:
            if (bgFifo.size == 0) {
                if (get_lcdc_flag(LCDCFlag::BG_WINDOW_ENABLE)) {
                    for (int i = 0; i < TILE_PX_SIZE; i++) {
                        u8 align = (1 << 7) >> i;

                        bool hi = fetcher.data1 & align;
                        bool lo = fetcher.data0 & align;
                        u8 col = (hi << 1) | lo;
                        bgFifo.push_tail({col, DMGPalette::BGP, false});
                    }
                } else {
                    for (int i = 0; i < TILE_PX_SIZE; i++) {
                        bgFifo.push_tail({0, DMGPalette::BGP, false});
                    }
                }
                fetcher.tileX++;
                fetcher.curFetchState = 7;
                if (line == 130) {
                    // printf("push-%d\n", ppuClocks);
                }
            }
            break;
        case Fetcher::SLEEP:
            if (line == 130) {
                // printf("sleep-%d\n", ppuClocks);
            }
            break;
    }
}

void PPU::sprite_fetch() {
    switch (fetcher.FETCH_STATES[fetcher.curFetchState]) {
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
        } break;
        case Fetcher::READ_TILE_0:
            fetcher.data0 = vramAddrBlock[fetcher.tileRowAddrOff];
            break;
        case Fetcher::READ_TILE_1:
            fetcher.data1 = vramAddrBlock[fetcher.tileRowAddrOff + 1];
        case Fetcher::PUSH: {
            bool bgPriority = fetcher.curSprite->flags & (1 << 7);
            u8 palette = ((fetcher.curSprite->flags & (1 << 4)) != 0) + DMGPalette::OBP0;
            bool xFlip = fetcher.curSprite->flags & (1 << 5);
            u8 fifoSize = spriteFifo.size;
            for (int i = 0; i < TILE_PX_SIZE; i++) {
                u8 align;
                if (xFlip) {
                    align = 1 << i;
                } else {
                    align = (1 << 7) >> i;
                }
                bool hi = fetcher.data1 & align;
                bool lo = fetcher.data0 & align;
                u8 col = (hi << 1) | lo;

                if (fifoSize > i) {
                    if (spriteFifo.get(i)->colIndex == 0) {
                        spriteFifo.set(i, {col, palette, bgPriority});
                    }
                } else {
                    spriteFifo.push_tail({col, palette, bgPriority});
                }
            }
            fetcher.curSprite = nullptr;
            fetcher.curFetchState = 7;
        } break;
        case Fetcher::SLEEP:
            break;
    }
}

bool PPU::get_lcdc_flag(LCDCFlag flag) { return *lcdc & (1 << (u8)flag); }
