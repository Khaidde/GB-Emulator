#include "apu.hpp"

#include <math.h>

#include "memory.hpp"

void VolumeEnvelope::set_nrx2(u8 regVal) {
    startingVolume = regVal >> 4;
    volume = startingVolume;
    volumeAddMode = (regVal & 0x8) >> 3;
    clockLen = regVal & 0x7;
    clockCnt = clockLen;
}

void VolumeEnvelope::trigger() {
    clockCnt = clockLen;
    volume = startingVolume;
}

void VolumeEnvelope::emulate_clock() {
    if (clockCnt > 0) {
        clockCnt--;
        if (clockCnt == 0) {
            clockCnt = clockLen;

            if (volumeAddMode && volume < 15) {
                volume++;
            } else if (volume > 0) {
                volume--;
            }
        }
    }
}

u8 VolumeEnvelope::get_volume() { return volume; }

void LengthCounter::set_disable_target(bool& enabled) { this->enabled = &enabled; }

void LengthCounter::set_length_enabled(bool enabled) { lengthEnabled = enabled; }

void LengthCounter::trigger() {
    if (lengthEnabled && lengthCnt == 0) {
        lengthCnt = cycleLen;
        printf("TODO check square channel length count trigger? %d\n", lengthCnt);
    }
}

void LengthCounter::emulate_clock() {
    if (lengthCnt > 0 && lengthEnabled) {
        lengthCnt--;
        if (lengthCnt == 0) {
            *enabled = false;
        }
    }
}

SquareChannel::SquareChannel() { lengthCounter.set_disable_target(enabled); }

void SquareChannel::set_frequency(u16 freq) { clockLen = (2048 - freq) << 2; }

void SquareChannel::set_duty(u8 duty) { dutyOff = duty << 3; }

void SquareChannel::set_dac_enable(bool dacEnabled) { this->dacEnabled = dacEnabled; }

void SquareChannel::trigger_event() {
    enabled = true;
    lengthCounter.trigger();
    volumeEnvelope.trigger();
    clockCnt = clockLen;
}

namespace {

constexpr bool DUTY_CYCLES[4 * 8] = {
    false, false, false, false, false, false, false, true,   // 12.5%
    true,  false, false, false, false, false, false, true,   // 25%
    true,  false, false, false, false, true,  true,  true,   // 50%
    false, true,  true,  true,  true,  true,  true,  false,  // 75%
};

}

void SquareChannel::emulate_clock() {
    if (clockCnt > 0) {
        clockCnt--;
        if (clockCnt == 0) {
            clockCnt = clockLen;
            cycleIndex = (cycleIndex + 1) & 0x7;

            if (enabled && dacEnabled) {
                outVol = volumeEnvelope.get_volume() * DUTY_CYCLES[cycleIndex + dutyOff];
            } else {
                outVol = 0;
            }
        }
    }
}

NoiseChannel::NoiseChannel() { lengthCounter.set_disable_target(enabled); }

void NoiseChannel::set_dac_enable(bool dacEnabled) { this->dacEnabled = dacEnabled; }

void NoiseChannel::set_nr43(u8 regVal) {
    u8 divisorCode = regVal & 0x7;
    u8 divisor;
    if (divisorCode == 0) {
        divisor = 8;
    } else {
        divisor = 16 * divisorCode;
    }
    widthMode = regVal & 0x8;
    clockLen = divisor << (regVal >> 4);
}

void NoiseChannel::trigger_event() {
    enabled = true;
    lsfr = 0x7FFF;
    lengthCounter.trigger();
    volumeEnvelope.trigger();
    clockCnt = clockLen;
}

void NoiseChannel::emulate_clock() {
    if (clockCnt > 0) {
        clockCnt--;
        if (clockCnt == 0) {
            clockCnt = clockLen;

            u8 res = ((lsfr >> 1) ^ lsfr) & 0x1;
            lsfr = res << 10 | (lsfr >> 1);
            if (widthMode) {
                lsfr = (lsfr & ~0x40) | (res << 6);
            }

            if (enabled && dacEnabled && (lsfr & 0x1) == 0) {
                outVol = volumeEnvelope.get_volume();
            } else {
                outVol = 0;
            }
        }
    }
}

APU::APU(Memory* memory) : memory(memory) {
    nr10 = &memory->ref(IOReg::NR10_REG);
    nr11 = &memory->ref(IOReg::NR11_REG);
    nr12 = &memory->ref(IOReg::NR12_REG);
    nr13 = &memory->ref(IOReg::NR13_REG);
    nr14 = &memory->ref(IOReg::NR14_REG);

    nr21 = &memory->ref(IOReg::NR21_REG);
    nr22 = &memory->ref(IOReg::NR22_REG);
    nr23 = &memory->ref(IOReg::NR23_REG);
    nr24 = &memory->ref(IOReg::NR24_REG);

    nr50 = &memory->ref(IOReg::NR50_REG);
    nr51 = &memory->ref(IOReg::NR51_REG);
    nr52 = &memory->ref(IOReg::NR52_REG);
}

void APU::restart() {
    *nr10 = 0x80;
    *nr11 = 0xBF;
    *nr12 = 0xF3;
    *nr13 = 0xC1;
    *nr14 = 0x14;

    *nr21 = 0xFF;
    *nr22 = 0x3F;
    *nr23 = 0x00;
    *nr24 = 0xB8;

    memory->ref(IOReg::NR41_REG) = 0xFF;
    memory->ref(IOReg::NR42_REG) = 0x00;
    memory->ref(IOReg::NR43_REG) = 0x00;
    memory->ref(IOReg::NR44_REG) = 0xBF;

    *nr50 = 0x00;
    *nr51 = 0x00;
    *nr52 = 0x70;

    frameSequenceClocks = 0;
}

void APU::sample(s16* sampleBuffer, u16 sampleLen) {
    for (int i = 0; i < sampleLen; i += 2) {
        sampleBuffer[i] = sampleQueue.dequeue();
        sampleBuffer[i + 1] = sampleQueue.dequeue();
    }
}

constexpr s16 MASTER_VOLUME = 10000;
constexpr u16 SAMPLE_RATE = 44100;
constexpr double CLOCKS_PER_SAMPLE = 70224 / (SAMPLE_RATE / (1000.0 / 16.0));
static double downSample = 0;
void APU::emulate_clock() {
    // frame sequencer ticks at 512HZ so 4194304 / 512 = 8192
    frameSequenceClocks++;
    if (frameSequenceClocks == 8192) {
        frameSequenceClocks = 0;
        switch (frameSequenceStep) {
            case 0:
                square1.lengthCounter.emulate_clock();
                square2.lengthCounter.emulate_clock();
                noise.lengthCounter.emulate_clock();
                break;
            case 1:
                break;
            case 2:
                square1.lengthCounter.emulate_clock();
                square2.lengthCounter.emulate_clock();
                noise.lengthCounter.emulate_clock();
                break;
            case 3:
                break;
            case 4:
                square1.lengthCounter.emulate_clock();
                square2.lengthCounter.emulate_clock();
                noise.lengthCounter.emulate_clock();
                break;
            case 5:
                break;
            case 6:
                square1.lengthCounter.emulate_clock();
                square2.lengthCounter.emulate_clock();
                noise.lengthCounter.emulate_clock();
                break;
            case 7:
                square1.volumeEnvelope.emulate_clock();
                square2.volumeEnvelope.emulate_clock();
                noise.volumeEnvelope.emulate_clock();
                break;
        }
        frameSequenceStep = (frameSequenceStep + 1) & 0x7;
    }

    square1.emulate_clock();
    square2.emulate_clock();
    noise.emulate_clock();

    if (downSample >= CLOCKS_PER_SAMPLE) {
        downSample -= CLOCKS_PER_SAMPLE;

        double mixedLeftVol = 0;
        if (*nr51 & 0x10) {
            mixedLeftVol += square1.get_out_vol();
        }
        if (*nr51 & 0x20) {
            mixedLeftVol += square2.get_out_vol();
        }
        if (*nr51 & 0x40) {
            mixedLeftVol += noise.get_out_vol();
        }
        mixedLeftVol *= (leftVol + 1);
        sampleQueue.enqueue(MASTER_VOLUME * mixedLeftVol / 240.0);

        double mixedRightVol = 0;
        if (*nr51 & 0x1) {
            mixedRightVol += square1.get_out_vol();
        }
        if (*nr51 & 0x2) {
            mixedRightVol += square2.get_out_vol();
        }
        if (*nr51 & 0x4) {
            mixedRightVol += noise.get_out_vol();
        }
        mixedRightVol *= (rightVol + 1);
        sampleQueue.enqueue(MASTER_VOLUME * mixedRightVol / 240.0);
    }
    downSample++;
}

void APU::update_nr1x(u8 x, u8 val) {
    switch (x) {
        case 0:
            break;
        case 1:
            square1.set_duty((val & 0xC0) >> 6);
            square1.lengthCounter.set_load<64>(val & 0x2F);
            break;
        case 2:
            square1.set_dac_enable((val & 0xF8) != 0);

            square1.volumeEnvelope.set_nrx2(val);
            break;
        case 3:
            square1.set_frequency((*nr14 & 0x7) << 8 | val);
            break;
        case 4:
            square1.set_frequency((val & 0x7) << 8 | *nr13);
            square1.lengthCounter.set_length_enabled((val & 0x40) != 0);
            if (val >> 7) {
                square1.trigger_event();
            }
            break;
    }
}

void APU::update_nr2x(u8 x, u8 val) {
    switch (x) {
        case 1:
            square2.set_duty((val & 0xC0) >> 6);
            square2.lengthCounter.set_load<64>(val & 0x2F);
            break;
        case 2:
            square2.set_dac_enable((val & 0xF8) != 0);

            square2.volumeEnvelope.set_nrx2(val);
            break;
        case 3:
            square2.set_frequency((*nr24 & 0x7) << 8 | val);
            break;
        case 4:
            square2.set_frequency((val & 0x7) << 8 | *nr23);
            square2.lengthCounter.set_length_enabled((val & 0x40) != 0);
            if (val >> 7) {
                square2.trigger_event();
            }
            break;
    }
}

void APU::update_nr4x(u8 x, u8 val) {
    switch (x) {
        case 1:
            noise.lengthCounter.set_load<64>(val & 0x2F);
            break;
        case 2:
            noise.set_dac_enable((val & 0xF8) != 0);

            noise.volumeEnvelope.set_nrx2(val);
            break;
        case 3:
            noise.set_nr43(val);
            break;
        case 4:
            if (val >> 7) {
                noise.trigger_event();
            }
            break;
    }
}
void APU::update_lr_enable(u8 lrEnableRegister) {
    leftVol = (lrEnableRegister & 0x70) >> 4;
    rightVol = lrEnableRegister & 0x7;
}