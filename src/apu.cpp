#include "apu.hpp"

#include "memory.hpp"

template <int maxLoad>
void LengthCounter<maxLoad>::restart() {
    lengthEnabled = false;
    lengthCnt = 0;
}

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
    lengthCnt = maxLoad - lengthLoad;
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

void VolumeEnvelope::restart() {
    volume = 0;
    startingVolume = 0;
    volumeAddMode = false;

    envelopeCnt = 0;
    clockLen = 0;
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

void SquareChannel::restart() {
    lengthCounter.restart();
    volumeEnvelope.restart();
    cycleIndex = 0;
    clockCnt = 0;
    clockLen = 0;
}

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

            outVol = enabled * dacEnabled * volumeEnvelope.get_volume() *
                     DUTY_CYCLES[cycleIndex + dutyOff];
        }
    }
}

void SquareChannel::boot_length_counter() { lengthCounter.set_load(0); }

bool SquareChannel::is_length_active() { return lengthCounter.is_active(); }

u8 SquareChannel::get_pcm() { return outVol; }

u8 SquareChannel::get_left_vol() { return outVol * leftEnable; }

u8 SquareChannel::get_right_vol() { return outVol * rightEnable; }

WaveChannel::WaveChannel() { lengthCounter.set_disable_target(enabled); }

void WaveChannel::restart() { lengthCounter.restart(); }

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

u8 WaveChannel::get_amplitude() { return outVol; }

u8 WaveChannel::get_left_vol() { return outVol * leftEnable; }

u8 WaveChannel::get_right_vol() { return outVol * rightEnable; }

NoiseChannel::NoiseChannel() { lengthCounter.set_disable_target(enabled); }

void NoiseChannel::restart() {
    lengthCounter.restart();
    volumeEnvelope.restart();
}

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
            u8 divisor = (val & 0x7) << 4;
            if (divisor == 0) divisor = 8;
            widthMode = (val & 0x8) != 0;
            clockLen = divisor << (val >> 4);
            if (val >> 4 >= 14) {
                clockLen = 0;
            }
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

            bool res = ((lsfr >> 1) ^ lsfr) & 0x1;
            lsfr = (res << 14) | ((lsfr >> 1) & 0x3FFF);
            if (widthMode) {
                lsfr = (lsfr & 0xFFBF) | (res << 6);
            }

            outVol = enabled * dacEnabled * volumeEnvelope.get_volume() * ((lsfr & 0x1) == 0);
        }
    }
}

bool NoiseChannel::is_length_active() { return lengthCounter.is_active(); }

u8 NoiseChannel::get_amplitude() { return outVol; }

u8 NoiseChannel::get_left_vol() { return outVol * leftEnable; }

u8 NoiseChannel::get_right_vol() { return outVol * rightEnable; }

APU::APU(Memory& memory) : memory(&memory) {
    wave.set_samples(&memory.ref(IOReg::WAVE_TABLE_START_REG));
}

void APU::restart() {
    square1.restart();
    square2.restart();
    wave.restart();
    noise.restart();

    // Simulate initial length counter after boot rom
    square1.boot_length_counter();

    frameSequenceClocks = 0;
    downSampleCnt = 0;

    isPowerOn = true;
}

void APU::sample(s16* sampleBuffer, u16 sampleLen) {
    for (int i = 0; i < sampleLen; i += 2) {
        if (sampleQueue.size == 0) break;
        sampleBuffer[i] = sampleQueue.dequeue();
        sampleBuffer[i + 1] = sampleQueue.dequeue();
    }
}

constexpr double CLOCKS_PER_SAMPLE =
    PPU::TOTAL_CLOCKS / (Constants::SAMPLE_RATE / (1000.0 / Constants::MS_PER_FRAME));
void APU::emulate_clock() {
    if (!isPowerOn) {
        return;
    }
    frameSequenceClocks++;
    if (frameSequenceClocks == 8192) {
        frameSequenceClocks = 0;
        if ((frameSequenceStep & 0x1) == 0x0) {
            square1.emulate_length_clock();
            square2.emulate_length_clock();
            wave.emulate_length_clock();
            noise.emulate_length_clock();
        }
        if ((frameSequenceStep & 0x3) == 0x2) {
            square1.emulate_sweep_clock();
        }
        if (frameSequenceStep == 0x7) {
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
        sampleQueue.enqueue((s16)(Constants::MASTER_VOLUME * mixedLeftVol * leftVol));

        u16 mixedRightVol = 0;
        mixedRightVol += square1.get_right_vol();
        mixedRightVol += square2.get_right_vol();
        mixedRightVol += wave.get_right_vol();
        mixedRightVol += noise.get_right_vol();
        sampleQueue.enqueue((s16)(Constants::MASTER_VOLUME * mixedRightVol * rightVol));
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
        u8 res = (isPowerOn << 7) | 0x70;
        res |= square1.is_length_active();
        res |= square2.is_length_active() << 1;
        res |= wave.is_length_active() << 2;
        res |= noise.is_length_active() << 3;
        return res;
    }
    return originalVal | REGISTER_MASK[ioReg - 0x10];
}

void APU::write_register(u8 ioReg, u8 val) {
    if (ioReg == 0x26) {
        isPowerOn = val & 0x80;
        if (!isPowerOn) {
            for (int i = 0xFF10; i <= 0xFF25; i++) {
                memory->write(i, 0);
            }
            square1.restart();
            square2.restart();

            leftVol = 1 << 4;
            rightVol = 1 << 4;
            square1.leftEnable = square1.rightEnable = false;
            square2.leftEnable = square2.rightEnable = false;
            wave.leftEnable = wave.rightEnable = false;
            noise.leftEnable = noise.rightEnable = false;

            frameSequenceClocks = 0;
            downSampleCnt = 0;
        }
    }
    if (!isPowerOn) {
        return;
    }
    if (ioReg == 0x24) {
        leftVol = (1 + ((val & 0x70) >> 4)) << 4;
        rightVol = (1 + (val & 0x7)) << 4;
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

u8 APU::read_pcm12() { return square1.get_pcm() | square2.get_pcm() << 4; }

u8 APU::read_pcm34() { return wave.get_amplitude() | noise.get_amplitude() << 4; }
