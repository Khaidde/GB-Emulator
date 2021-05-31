#include "apu.hpp"

#include <math.h>

#include "memory.hpp"

template <int maxLoad>
void LengthCounter<maxLoad>::set_disable_target(bool& enabled) {
    this->enabled = &enabled;
}

template <int maxLoad>
void LengthCounter<maxLoad>::set_length_enabled(bool enabled) {
    lengthEnabled = enabled;
}

template <int maxLoad>
void LengthCounter<maxLoad>::set_load(u8 lengthLoad) {
    cycleLen = maxLoad - lengthLoad;
    lengthCnt = cycleLen;
}

template <int maxLoad>
void LengthCounter<maxLoad>::trigger() {
    if (lengthEnabled && lengthCnt == 0) {
        lengthCnt = maxLoad;
    }
}

template <int maxLoad>
void LengthCounter<maxLoad>::emulate_clock() {
    if (lengthCnt > 0 && lengthEnabled) {
        lengthCnt--;
        if (lengthCnt == 0) {
            *enabled = false;
        }
    }
}

template <int maxLoad>
bool LengthCounter<maxLoad>::is_active() {
    return lengthCnt > 0;
}

void VolumeEnvelope::write_volume_registers(u8 val) {
    startingVolume = val >> 4;
    volumeAddMode = (val & 0x8) != 0;
    clockLen = val & 0x7;
}

void VolumeEnvelope::trigger() {
    envelopeCnt = clockLen;
    volume = startingVolume;
}

void VolumeEnvelope::emulate_clock() {
    if (envelopeCnt > 0) {
        envelopeCnt--;
        if (envelopeCnt == 0) {
            envelopeCnt = clockLen;
            volume = volumeAddMode ? (volume + (volume < 15)) : (volume - (volume > 0));
        }
    }
}

u8 VolumeEnvelope::get_volume() { return volume; }

SquareChannel::SquareChannel() { lengthCounter.set_disable_target(enabled); }

void SquareChannel::write_registers(char regType, u8 val) {
    switch (regType) {
        case 0:
            sweepClockLen = (val & 0x70) >> 4;
            negate = (val & 0x8) != 0;
            sweepShift = val & 0x7;
            break;
        case 1:
            dutyOff = (val & 0xC0) >> 3;
            lengthCounter.set_load(val & 0x3F);
            break;
        case 2:
            dacEnabled = (val & 0xF8) != 0;
            volumeEnvelope.write_volume_registers(val);
            break;
        case 3:
            freqLSB = val;
            update_frequency();
            break;
        case 4:
            freqMSB = val & 0x7;
            update_frequency();
            lengthCounter.set_length_enabled((val & 0x40) != 0);
            if (val >> 7) {
                enabled = true;
                lengthCounter.trigger();
                volumeEnvelope.trigger();

                shadowFrequency = frequency;
                clockLen = (2048 - frequency) << 2;
                clockCnt = (clockLen & 0xFFFC) | (clockCnt & 0x3);
                sweepEnabled = (sweepShift > 0 || sweepClockLen > 0);
                sweepClockCnt = sweepClockLen;
            }
            break;
    }
}

void SquareChannel::update_frequency() {
    frequency = (freqMSB << 8) | freqLSB;
    shadowFrequency = frequency;
    clockLen = (2048 - frequency) << 2;
}

void SquareChannel::emulate_sweep_clock() {
    if (sweepClockCnt > 0) {
        sweepClockCnt--;
        if (sweepClockCnt == 0) {
            sweepClockCnt = sweepClockLen;
            if (sweepEnabled && sweepClockLen != 0) {
                u16 newFrequency;
                u16 newFrequency2;
                if (negate) {
                    newFrequency = shadowFrequency - (shadowFrequency >> sweepShift);
                    newFrequency2 = newFrequency - (newFrequency >> sweepShift);
                } else {
                    newFrequency = shadowFrequency + (shadowFrequency >> sweepShift);
                    newFrequency2 = newFrequency + (newFrequency >> sweepShift);
                }
                if (newFrequency <= 2047 && sweepShift != 0) {
                    shadowFrequency = newFrequency;
                    clockLen = (2048 - shadowFrequency) << 2;
                }
                if (newFrequency > 2047 || newFrequency2 > 2047) {
                    enabled = false;
                }
            }
        }
    }
}

void SquareChannel::emulate_length_clock() { lengthCounter.emulate_clock(); }

void SquareChannel::emulate_volume_clock() { volumeEnvelope.emulate_clock(); }

static constexpr bool DUTY_CYCLES[4 * 8] = {
    false, false, false, false, false, false, false, true,   // 12.5%
    true,  false, false, false, false, false, false, true,   // 25%
    true,  false, false, false, false, true,  true,  true,   // 50%
    false, true,  true,  true,  true,  true,  true,  false,  // 75%
};
void SquareChannel::emulate_clock() {
    if (clockCnt > 0) {
        clockCnt--;
        if (clockCnt == 0) {
            clockCnt = clockLen;
            cycleIndex = (cycleIndex + 1) & 0x7;

            outVol = enabled * dacEnabled * volumeEnvelope.get_volume() * DUTY_CYCLES[cycleIndex + dutyOff];
        }
    }
}

bool SquareChannel::is_length_active() { return lengthCounter.is_active(); }

u8 SquareChannel::get_left_vol() { return outVol * leftEnable; }

u8 SquareChannel::get_right_vol() { return outVol * rightEnable; }

WaveChannel::WaveChannel() { lengthCounter.set_disable_target(enabled); }

void WaveChannel::set_samples(u8* samples) { this->samples = samples; }

void WaveChannel::write_registers(char regType, u8 val) {
    switch (regType) {
        case 0:
            dacEnabled = (val & 0x80) != 0;
            break;
        case 1:
            lengthCounter.set_load(val);
            break;
        case 2:
            shiftVol = ((val & 0x60) >> 5) - 1;
            if (shiftVol > 2) {
                shiftVol = 4;
            }
            break;
        case 3:
            freqLSB = val;
            update_frequency();
            break;
        case 4:
            freqMSB = val & 0x7;
            update_frequency();
            lengthCounter.set_length_enabled((val & 0x40) != 0);
            if (val >> 7) {
                enabled = true;
                lengthCounter.trigger();
                clockCnt = clockLen;
                sampleIndex = 0;
            }
            break;
    }
}

void WaveChannel::update_frequency() {
    u16 frequency = (freqMSB << 8) | freqLSB;
    clockLen = (2048 - frequency) << 1;
}

void WaveChannel::emulate_length_clock() { lengthCounter.emulate_clock(); }

void WaveChannel::emulate_clock() {
    if (clockCnt > 0) {
        clockCnt--;
        if (clockCnt == 0) {
            clockCnt = clockLen;
            sampleIndex = (sampleIndex + 1) & 0x1F;

            u8 sample;
            if ((sampleIndex & 0x1) == 0) {
                sample = samples[sampleIndex >> 1] >> 4;
            } else {
                sample = samples[sampleIndex >> 1] & 0xF;
            }

            outVol = enabled * dacEnabled * (sample >> shiftVol);
        }
    }
}

bool WaveChannel::is_length_active() { return lengthCounter.is_active(); }

u8 WaveChannel::get_left_vol() { return outVol * leftEnable; }

u8 WaveChannel::get_right_vol() { return outVol * rightEnable; }

NoiseChannel::NoiseChannel() { lengthCounter.set_disable_target(enabled); }

void NoiseChannel::write_registers(char regType, u8 val) {
    switch (regType) {
        case 1:
            lengthCounter.set_load(val & 0x3F);
            break;
        case 2:
            dacEnabled = (val & 0xF8) != 0;
            volumeEnvelope.write_volume_registers(val);
            break;
        case 3: {
            u8 divisor = 16 * (val & 0x7);
            if (divisor == 0) divisor = 8;
            widthMode = (val & 0x8) != 0;
            clockLen = divisor << (val >> 4);
            clockEnabled = (val >> 4) < 14;
        } break;
        case 4:
            lengthCounter.set_length_enabled((val & 0x40) != 0);
            if (val >> 7) {
                enabled = true;
                lsfr = 0x7FFF;
                lengthCounter.trigger();
                volumeEnvelope.trigger();
                clockCnt = clockLen;
            }
            break;
    }
}

void NoiseChannel::emulate_length_clock() { lengthCounter.emulate_clock(); }

void NoiseChannel::emulate_volume_clock() { volumeEnvelope.emulate_clock(); }

void NoiseChannel::emulate_clock() {
    if (clockEnabled && clockCnt > 0) {
        clockCnt--;
        if (clockCnt == 0) {
            clockCnt = clockLen;

            u8 res = ((lsfr >> 1) ^ lsfr) & 0x1;
            lsfr = (res << 14) | (lsfr >> 1);
            if (widthMode) {
                lsfr = (lsfr & ~0x0040) | (res << 6);
            }

            outVol = enabled * dacEnabled * volumeEnvelope.get_volume() * ((lsfr & 0x1) == 0);
        }
    }
}

bool NoiseChannel::is_length_active() { return lengthCounter.is_active(); }

u8 NoiseChannel::get_left_vol() { return outVol * leftEnable; }

u8 NoiseChannel::get_right_vol() { return outVol * rightEnable; }

APU::APU(Memory* memory) : memory(memory) { wave.set_samples(&memory->ref(IOReg::WAVE_TABLE_START_REG)); }

void APU::restart() {
    memory->write(IOReg::NR10_REG, 0x80);
    memory->write(IOReg::NR11_REG, 0xBF);
    memory->write(IOReg::NR12_REG, 0xF3);
    memory->write(IOReg::NR13_REG, 0xC1);

    memory->write(IOReg::NR32_REG, 0x9F);
    memory->write(IOReg::NR34_REG, 0xBF);

    memory->write(IOReg::NR43_REG, 0x00);

    memory->write(IOReg::NR50_REG, 0x77);
    memory->write(IOReg::NR51_REG, 0xF3);
    memory->write(IOReg::NR52_REG, 0xF1);

    frameSequenceClocks = 0;
    downSampleCnt = 0;
}

void APU::sample(s16* sampleBuffer, u16 sampleLen) {
    for (int i = 0; i < sampleLen; i += 2) {
        if (sampleQueue.size == 0) break;
        sampleBuffer[i] = sampleQueue.dequeue();
        sampleBuffer[i + 1] = sampleQueue.dequeue();
    }
}

constexpr double MASTER_VOLUME = 3;
constexpr u16 SAMPLE_RATE = 44100;
constexpr double CLOCKS_PER_SAMPLE = 70224.0 / (SAMPLE_RATE / (1000.0 / Constants::MS_PER_FRAME));
void APU::emulate_clock() {
    frameSequenceClocks++;
    if (frameSequenceClocks == 8192) {
        frameSequenceClocks = 0;
        if ((frameSequenceStep & 0x1) == 0) {
            square1.emulate_length_clock();
            square2.emulate_length_clock();
            wave.emulate_length_clock();
            noise.emulate_length_clock();
        }
        if ((frameSequenceStep & 0x3) == 0x2) {
            square1.emulate_sweep_clock();
        }
        if (frameSequenceStep == 7) {
            square1.emulate_volume_clock();
            square2.emulate_volume_clock();
            noise.emulate_volume_clock();
        }
        frameSequenceStep = (frameSequenceStep + 1) & 0x7;
    }

    square1.emulate_clock();
    square2.emulate_clock();
    wave.emulate_clock();
    noise.emulate_clock();

    if (downSampleCnt >= CLOCKS_PER_SAMPLE) {
        downSampleCnt -= CLOCKS_PER_SAMPLE;

        u16 mixedLeftVol = 0;
        mixedLeftVol += square1.get_left_vol();
        mixedLeftVol += square2.get_left_vol();
        mixedLeftVol += wave.get_left_vol();
        mixedLeftVol += noise.get_left_vol();
        sampleQueue.enqueue(MASTER_VOLUME * mixedLeftVol * leftVol);

        u16 mixedRightVol = 0;
        mixedRightVol += square1.get_right_vol();
        mixedRightVol += square2.get_right_vol();
        mixedRightVol += wave.get_right_vol();
        mixedRightVol += noise.get_right_vol();
        sampleQueue.enqueue(MASTER_VOLUME * mixedRightVol * rightVol);
    }
    downSampleCnt++;
}

static constexpr u8 REGISTER_MASK[5 * 4] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF,  // NR1x
    0xFF, 0x3F, 0x00, 0xFF, 0xBF,  // NR2x
    0x7F, 0xFF, 0x9F, 0xFF, 0xBF,  // NR3x
    0xFF, 0xFF, 0x00, 0x00, 0xBF,  // NR3x
};
u8 APU::read_register(u8 originalVal, u8 ioReg) {
    if (ioReg == 0x24 || ioReg == 0x25) {
        return originalVal;
    }
    if (ioReg == 0x26) {
        u8 res = (originalVal & 0x80) | 0x70;
        res |= square1.is_length_active();
        res |= square2.is_length_active() << 1;
        res |= wave.is_length_active() << 2;
        res |= noise.is_length_active() << 3;
        return res;
    }
    return originalVal | REGISTER_MASK[ioReg - 0x10];
}

void APU::write_register(u8 ioReg, u8 val) {
    if (ioReg == 0x24) {
        leftVol = (1 + ((val & 0x70) >> 4)) * 16;
        rightVol = (1 + (val & 0x7)) * 16;
    } else if (ioReg == 0x25) {
        square1.leftEnable = val & 0x10;
        square2.leftEnable = val & 0x20;
        wave.leftEnable = val & 0x40;
        noise.leftEnable = val & 0x80;

        square1.rightEnable = val & 0x1;
        square2.rightEnable = val & 0x2;
        wave.rightEnable = val & 0x4;
        noise.rightEnable = val & 0x8;
    } else {
        char channel = (ioReg - 0x10) / 5;
        char index = (ioReg - 0x10) % 5;
        switch (channel) {
            case 0:
                square1.write_registers(index, val);
                break;
            case 1:
                square2.write_registers(index, val);
                break;
            case 2:
                wave.write_registers(index, val);
                break;
            case 3:
                noise.write_registers(index, val);
                break;
        }
    }
}