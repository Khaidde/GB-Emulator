#pragma once

#include "utils.hpp"

class Memory;

struct SampleQueue {
    void enqueue(s16 val) { data[(offset + size++) % CAPACITY] = val; }
    s16 dequeue() {
        s16 val = data[offset];
        offset = (offset + 1) % CAPACITY;
        size--;
        return val;
    }

    static constexpr short CAPACITY = 2048;
    u16 offset = 0;
    u32 size = 0;
    s16 data[CAPACITY];
};

template <int maxLoad>
class LengthCounter {
   public:
    void set_disable_target(bool& enabled);
    void set_length_enabled(bool enabled);
    void set_load(u8 lengthLoad);

    void trigger();
    void emulate_clock();

    bool is_active();

   private:
    bool* enabled;

    bool lengthEnabled = false;

    u16 lengthCnt;
    u16 cycleLen;
};

class VolumeEnvelope {
   public:
    void write_volume_registers(u8 val);

    void trigger();
    void emulate_clock();

    u8 get_volume();

   private:
    u8 volume;
    u8 startingVolume;
    bool volumeAddMode;

    u16 envelopeCnt = 0;
    u8 clockLen = 0;
};

class SquareChannel {
   public:
    SquareChannel();
    void write_registers(char regType, u8 val);
    void update_frequency();

    void emulate_sweep_clock();
    void emulate_length_clock();
    void emulate_volume_clock();
    void emulate_clock();

    bool is_length_active();

    u8 get_left_vol();
    u8 get_right_vol();

    bool leftEnable;
    bool rightEnable;

   private:
    bool enabled = false;
    bool dacEnabled = false;

    u8 sweepClockCnt = 0;
    u8 sweepClockLen = 0;
    bool negate;
    u8 sweepShift;
    bool sweepEnabled;
    u16 shadowFrequency;

    LengthCounter<64> lengthCounter;
    VolumeEnvelope volumeEnvelope;

    u8 cycleIndex = 0;
    u16 clockCnt = 0;
    u16 clockLen = 0;
    u8 dutyOff;

    u8 freqMSB;
    u8 freqLSB;
    u16 frequency;

    u8 outVol;
};

class WaveChannel {
   public:
    WaveChannel();
    void set_samples(u8* samples);
    void write_registers(char regType, u8 val);
    void update_frequency();

    void emulate_length_clock();
    void emulate_clock();

    bool is_length_active();

    u8 get_left_vol();
    u8 get_right_vol();

    bool leftEnable;
    bool rightEnable;

   private:
    bool enabled;
    bool dacEnabled;

    LengthCounter<256> lengthCounter;

    u8 shiftVol;

    u16 clockCnt = 0;
    u16 clockLen = 0;
    u8 sampleIndex = 0;
    u8* samples;

    u8 freqMSB;
    u8 freqLSB;

    u8 outVol;
};

class NoiseChannel {
   public:
    NoiseChannel();
    void write_registers(char regType, u8 val);

    void emulate_length_clock();
    void emulate_volume_clock();
    void emulate_clock();

    bool is_length_active();

    u8 get_left_vol();
    u8 get_right_vol();

    bool leftEnable;
    bool rightEnable;

   private:
    bool enabled = false;
    bool dacEnabled = false;

    LengthCounter<64> lengthCounter;
    VolumeEnvelope volumeEnvelope;

    u16 clockCnt = 0;
    u16 clockLen = 0;
    bool widthMode;
    bool clockEnabled;
    u16 lsfr = 0x7FFF;

    u8 outVol;
};

class APU {
   public:
    APU(Memory* memory);
    void restart();
    void sample(s16* sampleBuffer, u16 sampleLen);
    void emulate_clock();

    u8 read_register(u8 originalVal, u8 ioReg);
    void write_register(u8 ioReg, u8 val);

   private:
    Memory* memory;

    short frameSequenceClocks;
    char frameSequenceStep;
    double downSampleCnt;
    SampleQueue sampleQueue;

    SquareChannel square1;
    SquareChannel square2;
    WaveChannel wave;
    NoiseChannel noise;

    u8 leftVol;
    u8 rightVol;
};