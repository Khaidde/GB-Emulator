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
}

void PPU::restart() {
    bufferSel = 0;
    for (int i = 0; i < Constants::WIDTH * Constants::HEIGHT; i++) {
        (frameBuffers[0])[i] = BLANK_COLOR;
        (frameBuffers[1])[i] = BLANK_COLOR;
    }

    bgAutoIncrement = true;
    bgPaletteIndex = 0;

    curVramBank = &tileMapVram;

    line = 0x99;
    curPPUState = PPUState::V_BLANK;
    // initial drawn clocks at PC=0x100 with 1 cycle cpu prefetch accounted for (DMG)
    clockCnt = 56;
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

                if (!(*stat & 0x4) && (*ly == *lyc)) {
                    // only trigger lyc intr on rising edge of coincidence when enabling lcd
                    try_lyc_intr();
                    try_trigger_stat();
                }
                update_coincidence();
            } else if (!lcdOn && get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
                *stat = (*stat & 0xFC) | 0;
                *ly = 0;
                line = 0;
                statIntrFlags = 0;

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
            if (get_lcdc_flag(LCDCFlag::LCD_ENABLE)) {
                update_coincidence();
                try_lyc_intr();
                try_trigger_stat();
            }
            break;
        case IOReg::BGPI_REG:
            bgAutoIncrement = val & (1 << 7);
            bgPaletteIndex = val & 0x3F;
            break;
        case IOReg::BGPD_REG:
            bgPaletteData[bgPaletteIndex] = val;
            if (bgAutoIncrement) {
                bgPaletteIndex = (bgPaletteIndex + 1) & 0x3F;
            }
            break;
        case IOReg::OBPI_REG:
            obAutoIncrement = val & (1 << 7);
            obPaletteIndex = val & 0x3F;
            break;
        case IOReg::OBPD_REG:
            obPaletteData[obPaletteIndex] = val;
            if (obAutoIncrement) {
                obPaletteIndex = (obPaletteIndex + 1) & 0x3F;
            }
            break;
    }
}

void PPU::write_vbk(u8 val) { curVramBank = (val == 0) ? &tileMapVram : &tileAttribVram; }

void PPU::write_vram(u16 addr, u8 val) {
    if (0x8000 <= addr && addr < 0xA000) {
        if (!vramBlockWrite) {
            (*curVramBank)[addr - 0x8000] = val;
        }
    } else {
        fatal("Invalid VRAM write address: %04x\n", addr);
    }
}

u8 PPU::read_vram(u16 addr) {
    if (0x8000 <= addr && addr < 0xA000) {
        if (vramBlockRead) {
            return 0xFF;
        }
        return (*curVramBank)[addr - 0x8000];
    } else {
        fatal("Invalid VRAM read address: %04x\n", addr);
    }
}

bool PPU::is_oam_read_blocked() { return oamBlockRead; }

bool PPU::is_oam_write_blocked() { return oamBlockWrite; }

void PPU::render(u32* pixelBuffer) {
    FrameBuffer* curBuffer = &frameBuffers[!bufferSel];
    for (int i = 0; i < Constants::WIDTH * Constants::HEIGHT; i++) {
        pixelBuffer[i] = (*curBuffer)[i];
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

            // TODO check the timing wy trigger
            if (line == *wy) {
                fetcher.windowMode = true;
                fetcher.windowY = 0;
            }

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
            for (u8 i = 0; i < 40; i++) {
                u8* spritePtr = &oamAddrBlock[i << 2];
                u8 y = *spritePtr;
                u8 x = *(spritePtr + 1);

                u8 botBound = line + 16;
                u8 spriteHeight = get_lcdc_flag(LCDCFlag::SPRITE_SIZE) ? 16 : 8;
                if (x > 0 && botBound - spriteHeight < y && y <= botBound) {
                    u8 tileID = *(spritePtr + 2);
                    u8 flags = *(spritePtr + 3);
                    spriteList.add({i, y, x, tileID, flags});
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
    if (fetcher.windowMode && !fetcher.windowXEnable && curPixelX == *wx - 7) {
        if (is_window_enabled()) {
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
        bool isOBJPxl = false;
        BGFIFOData bgPxl = bgFifo.pop_head();
        u8 colIndex = bgPxl.colIndex;
        u8 paletteNum = bgPxl.paletteNum;
        if (spriteFifo.size > 0) {
            SpriteFIFOData spritePxl = spriteFifo.pop_head();
            bool masterSpritePriority =
                memory->is_CGB() && !get_lcdc_flag(LCDCFlag::BG_WINDOW_OR_PRIORITY_ENABLE);
            if (spritePxl.colIndex != 0) {
                if (masterSpritePriority || (!bgPxl.priority && !spritePxl.priority) ||
                    colIndex == 0) {
                    colIndex = spritePxl.colIndex;
                    paletteNum = spritePxl.paletteNum;
                    isOBJPxl = true;
                }
            }
        }

        if (curPixelX >= 0) {
            u32 col;
            if (memory->is_CGB()) {
                u8* paletteData = bgPaletteData;
                if (isOBJPxl) {
                    paletteData = obPaletteData;
                }
                u8 index = colIndex * 2 + 8 * paletteNum;
                u8 colLo = paletteData[index];
                u8 colHi = paletteData[index + 1];
                u8 r = colLo & 0x1F;
                u8 g = ((colHi & 3) << 3) | ((colLo & 0xE0) >> 5);
                u8 b = (colHi & 0x7C) >> 2;
                r *= 0x100 / 0x20;
                g *= 0x100 / 0x20;
                b *= 0x100 / 0x20;
                col = 0xFF000000 | (u32)(r << 0x10) | (u32)(g << 0x8) | b;
            } else {
                u8 i = (colIndex & 0x3) << 1;
                u8 mask = 3 << i;
                u8 palette = memory->read(IOReg::BGP_REG + paletteNum);
                col = baseColors[(palette & mask) >> i];
            }
            frameBuffers[bufferSel][line * Constants::WIDTH + curPixelX] = col;
        }
        curPixelX++;
    }
}

void PPU::background_fetch() {
    switch (fetcher.FETCH_STATES[fetcher.curFetchState]) {
        case Fetcher::READ_TILE_ID: {
            // TODO verify that flag is checked and turns off during READ_TILE_ID
            if (fetcher.windowXEnable && !is_window_enabled()) {
                fetcher.windowXEnable = false;
            }

            u16 tileMap = 0x1800;
            if (fetcher.windowXEnable) {
                if (get_lcdc_flag(LCDCFlag::WINDOW_TILE_SELECT)) {
                    tileMap = 0x1C00;
                }
            } else {
                if (get_lcdc_flag(LCDCFlag::BG_TILE_SELECT)) {
                    tileMap = 0x1C00;
                }
            }
            u16 xOff = fetcher.windowXEnable ? 0 : lockedSCX;
            fetcher.yOff = fetcher.windowXEnable ? fetcher.windowY : (*scy + line);

            u8 tileX = (xOff / TILE_PX_SIZE + fetcher.tileX) % TILESET_SIZE;
            u8 tileY = (fetcher.yOff / TILE_PX_SIZE) % TILESET_SIZE;

            u16 tileIndexAddr = (tileX + tileY * TILESET_SIZE) + tileMap;
            fetcher.tileIndex = tileMapVram[tileIndexAddr];

            if (memory->is_CGB()) {
                fetcher.tileAttribs = tileAttribVram[tileIndexAddr];
            }
        } break;
        case Fetcher::READ_TILE_0: {
            u16 tileByteAddr;
            if (get_lcdc_flag(LCDCFlag::TILE_DATA_SELECT)) {
                tileByteAddr = fetcher.tileIndex * TILE_MEM_LEN;
            } else {
                tileByteAddr = (u16)(0x1000 + (s8)fetcher.tileIndex * TILE_MEM_LEN);
            }
            if (memory->is_CGB()) {
                u8 y = fetcher.yOff & 0x7;
                if (fetcher.tileAttribs & (1 << 6)) {
                    tileByteAddr += (y ^ 0x7) << 1;
                } else {
                    tileByteAddr += y << 1;
                }
            } else {
                tileByteAddr += (fetcher.yOff % TILE_PX_SIZE) << 1;
            }
            fetcher.data0 =
                ((fetcher.tileAttribs & 0x8 && memory->is_CGB()) ? tileAttribVram
                                                                 : tileMapVram)[tileByteAddr];
        } break;
        case Fetcher::READ_TILE_1: {
            u16 tileByteAddr;
            if (get_lcdc_flag(LCDCFlag::TILE_DATA_SELECT)) {
                tileByteAddr = fetcher.tileIndex * TILE_MEM_LEN;
            } else {
                tileByteAddr = (u16)(0x1000 + (s8)fetcher.tileIndex * TILE_MEM_LEN);
            }
            if (memory->is_CGB()) {
                u8 y = fetcher.yOff & 0x7;
                if (fetcher.tileAttribs & (1 << 6)) {
                    tileByteAddr += (y ^ 0x7) << 1;
                } else {
                    tileByteAddr += y << 1;
                }
            } else {
                tileByteAddr += (fetcher.yOff % TILE_PX_SIZE) << 1;
            }
            fetcher.data1 =
                ((fetcher.tileAttribs & 0x8 && memory->is_CGB()) ? tileAttribVram
                                                                 : tileMapVram)[tileByteAddr + 1];
        }
        case Fetcher::PUSH:
            if (bgFifo.size == 0) {
                u8 palette;
                if (memory->is_CGB()) {
                    palette = fetcher.tileAttribs & 0x7;
                } else {
                    palette = DMGPalette::BGP;
                }
                bool priority = memory->is_CGB() && fetcher.tileAttribs & (1 << 7);
                if (memory->is_CGB() || get_lcdc_flag(LCDCFlag::BG_WINDOW_OR_PRIORITY_ENABLE)) {
                    bool xFlip = fetcher.tileAttribs & (1 << 5);
                    for (int i = 0; i < TILE_PX_SIZE; i++) {
                        u8 align = xFlip ? (1 << i) : (1 << 7) >> i;
                        bool hi = fetcher.data1 & align;
                        bool lo = fetcher.data0 & align;
                        u8 col = (hi << 1) | lo;

                        bgFifo.push_tail({col, palette, priority});
                    }
                } else {
                    for (int i = 0; i < TILE_PX_SIZE; i++) {
                        bgFifo.push_tail({0, palette, priority});
                    }
                }
                fetcher.tileX++;
                fetcher.curFetchState = 7;
            }
            break;
        case Fetcher::SLEEP:
            break;
    }
}

void PPU::sprite_fetch() {
    switch (fetcher.FETCH_STATES[fetcher.curFetchState]) {
        case Fetcher::READ_TILE_ID: {
            if (get_lcdc_flag(LCDCFlag::SPRITE_SIZE)) {
                fetcher.curSprite->tileID &= ~0x1;  // Make tileIndex even
            }
        } break;
        case Fetcher::READ_TILE_0: {
            u16 tileByteAddr = fetcher.curSprite->tileID * TILE_MEM_LEN;
            u8 yMask = get_lcdc_flag(LCDCFlag::SPRITE_SIZE) ? 0xF : 0x7;
            u8 y = (line - fetcher.curSprite->y) & yMask;
            if (fetcher.curSprite->flags & (1 << 6)) {
                tileByteAddr += (y ^ yMask) << 1;
            } else {
                tileByteAddr += y << 1;
            }
            fetcher.data0 =
                ((fetcher.curSprite->flags & 0x8 && memory->is_CGB()) ? tileAttribVram
                                                                      : tileMapVram)[tileByteAddr];
        } break;
        case Fetcher::READ_TILE_1: {
            u16 tileByteAddr = fetcher.curSprite->tileID * TILE_MEM_LEN;
            u8 yMask = get_lcdc_flag(LCDCFlag::SPRITE_SIZE) ? 0xF : 0x7;
            u8 y = (line - fetcher.curSprite->y) & yMask;
            if (fetcher.curSprite->flags & (1 << 6)) {
                tileByteAddr += (y ^ yMask) << 1;
            } else {
                tileByteAddr += y << 1;
            }
            fetcher.data1 = ((fetcher.curSprite->flags & 0x8 && memory->is_CGB())
                                 ? tileAttribVram
                                 : tileMapVram)[tileByteAddr + 1];
        }
        case Fetcher::PUSH: {
            bool bgPriority = fetcher.curSprite->flags & (1 << 7);
            u8 palette;
            if (memory->is_CGB()) {
                palette = fetcher.curSprite->flags & 0x7;
            } else {
                palette = ((fetcher.curSprite->flags & (1 << 4)) != 0) + DMGPalette::OBP0;
            }
            bool xFlip = fetcher.curSprite->flags & (1 << 5);
            u8 fifoSize = spriteFifo.size;
            for (int i = 0; i < TILE_PX_SIZE; i++) {
                u8 align = xFlip ? (1 << i) : (1 << 7) >> i;
                bool hi = fetcher.data1 & align;
                bool lo = fetcher.data0 & align;
                u8 col = (hi << 1) | lo;

                SpriteFIFOData cur = {col, palette, bgPriority, fetcher.curSprite->oamIndex};
                if (fifoSize > i) {
                    if (col != 0) {
                        // Sprites with lower oam index are rendered on top in GBC
                        bool oamIndexPriority = ((memory->read(IOReg::OPRI_REG) & 0x1) == 0 &&
                                                 spriteFifo.get(i)->oamIndex > cur.oamIndex);
                        if (oamIndexPriority || spriteFifo.get(i)->colIndex == 0) {
                            spriteFifo.set(i, std::move(cur));
                        }
                    }
                } else {
                    spriteFifo.push_tail(std::move(cur));
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

bool PPU::is_window_enabled() {
    return get_lcdc_flag(LCDCFlag::WINDOW_ENABLE) &&
           (memory->is_CGB() || get_lcdc_flag(LCDCFlag::BG_WINDOW_OR_PRIORITY_ENABLE));
}
