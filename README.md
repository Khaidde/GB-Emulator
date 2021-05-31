# GB-Emulator

A Gameboy/Gameboy Color emulator aimed at accuracy. This project is meant to be a learning exercise in C++. There are a ton of other gameboy emulators such as Gambatte, BGB and SameBoy which have better UIs, debuggers, and accuracy. I am doing this as a fun side project.

# TODO

-   OAM bug
-   ROM saving (traditional battery saving)
-   Save States (emulator state save)
-   MBC2, MBC3, MBC5
-   Support CGB (Gameboy Color)
-   Command-line parser

# ROM Tests

## Blargg tests

| Test         | Passed | Note            |
| ------------ | :----: | --------------- |
| cpu_instrs   |   x    |                 |
| dmg_sound    |        | APU unimp       |
| instr_timing |   x    |                 |
| mem_timing   |   x    |                 |
| mem_timing_2 |   x    |                 |
| oam_bug      |        | oam bug ignored |
| halt_bug     |   x    |                 |

## mooneye-gb tests

### bits

| Test           | Passed | Note |
| -------------- | :----: | ---- |
| mem_oam        |   x    |      |
| reg_f          |   x    |      |
| unused_hwio-GS |   x    |      |

### instr

| Test | Passed | Note |
| ---- | :----: | ---- |
| daa  |   x    |      |

### interrupts

| Test    | Passed | Note      |
| ------- | :----: | --------- |
| ie_push |        | Unhandled |

### oam_dma

| Test       | Passed | Note       |
| ---------- | :----: | ---------- |
| basic      |   x    |            |
| reg_read   |   x    |            |
| sources-GS |        | MBC5 unimp |

### ppu

| Test                        | Passed | Note            |
| --------------------------- | :----: | --------------- |
| hblank_ly_scx_timing-GS     |   x    |                 |
| intr_1_2_timing-GS          |   x    |                 |
| intr_2_0_timing             |   x    |                 |
| intr_2_mode0_timing         |   x    |                 |
| intr_2_mode0_timing_sprites |        | test #00 failed |
| intr_2_mode3_timing         |   x    |                 |
| intr_2_oam_ok_timing        |   x    |                 |
| lcdon_timing-GS             |        |                 |
| lcdon_write_timing-GS       |        |                 |
| stat_irq_blocking           |   x    |                 |
| stat_lyc_onoff              |        |                 |
| vblank_stat_intr-GS         |        |                 |

### acceptance

| Test                    | Passed | Note |
| ----------------------- | :----: | ---- |
| add_sp_e_timing         |   x    |      |
| boot_div-dmgABCmgb      |   x    |      |
| boot_hwio-dmgABCmgb     |   x    |      |
| boot_regs-dmgABC        |   x    |      |
| call_cc_timing          |   x    |      |
| call_cc_timing2         |   x    |      |
| call_timing             |   x    |      |
| call_timing2            |   x    |      |
| di_timing-GS            |   x    |      |
| div_timing              |   x    |      |
| ei_sequence             |   x    |      |
| halt_ime0_ei            |   x    |      |
| halt_ime0_nointr_timing |   x    |      |
| halt_ime1_timing        |   x    |      |
| halt_ime1_timing2-GS    |   x    |      |
| if_ie_registers         |   x    |      |
| intr_timing             |   x    |      |
| jp_cc_timing            |   x    |      |
| jp_timing               |   x    |      |
| ld_hl_sp_e_timing       |   x    |      |
| oam_dma_restart         |   x    |      |
| oam_dma_start           |   x    |      |
| oam_dma_timing          |   x    |      |
| pop_timing              |   x    |      |
| push_timing             |   x    |      |
| rapid_di_ei             |   x    |      |
| ret_cc_timing           |   x    |      |
| ret_timing              |   x    |      |
| reti_intr_timing        |   x    |      |
| reti_timing             |   x    |      |
| rst_timing              |   x    |      |

### mbc1

| Test              | Passed | Note        |
| ----------------- | :----: | ----------- |
| bits_bank1        |   x    |             |
| bits_bank2        |   x    |             |
| bits_mode         |   x    |             |
| bits_ramg         |   x    |             |
| multicart_rom_8Mb |        | Unsupported |
| ram_64kb          |   x    |             |
| ram_256kb         |   x    |             |
| rom_512kb         |   x    |             |
| rom_1Mb           |   x    |             |
| rom_2Mb           |   x    |             |
| rom_4Mb           |   x    |             |
| rom_8Mb           |   x    |             |
| rom_16Mb          |   x    |             |

### manual-only

| Test            | Passed | Note |
| --------------- | :----: | ---- |
| sprite_priority |   x    |      |
