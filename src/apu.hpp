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
    u16 size = 0;
    s16 data[CAPACITY];
};

class LengthCounter {
   public:
    void set_disable_target(bool& enabled);
    void set_length_enabled(bool enabled);

    template <int maxLoad>
    void set_load(u8 lengthLoad) {
        cycleLen = maxLoad - lengthLoad;
    }
    void trigger();
    void emulate_clock();

   private:
    bool* enabled;

    bool lengthEnabled = false;

    u16 lengthCnt;
    u16 cycleLen;
};

class VolumeEnvelope {
   public:
    void set_nrx2(u8 regVal);

    void trigger();
    void emulate_clock();

    u8 get_volume();

   private:
    u8 volume;
    u8 startingVolume;
    bool volumeAddMode;

    u16 clockCnt = 0;
    u16 clockLen = 0;
};

class SquareChannel {
   public:
    SquareChannel();
    void set_dac_enable(bool dacEnabled);
    void set_frequency(u16 freq);
    void set_duty(u8 duty);

    void trigger_event();
    void emulate_clock();

    u8 get_out_vol() { return outVol; }

    LengthCounter lengthCounter;
    VolumeEnvelope volumeEnvelope;

   private:
    bool enabled = false;
    bool dacEnabled = false;

    u8 cycleIndex = 0;
    u16 clockCnt = 0;
    u16 clockLen = 0;
    u8 dutyOff;

    u8 outVol;
};

class NoiseChannel {
   public:
    NoiseChannel();
    void set_dac_enable(bool dacEnabled);
    void set_nr43(u8 regVal);

    void trigger_event();
    void emulate_clock();

    u8 get_out_vol() { return outVol; }

    LengthCounter lengthCounter;
    VolumeEnvelope volumeEnvelope;

   private:
    bool enabled = false;
    bool dacEnabled = false;

    u16 clockCnt = 0;
    u16 clockLen = 0;
    bool widthMode;
    u16 lsfr = 0x7FFF;

    u8 outVol;
};

class APU {
   public:
    APU(Memory* memory);
    void restart();
    void sample(s16* sampleBuffer, u16 sampleLen);
    void emulate_clock();

    void update_nr1x(u8 x, u8 val);
    void update_nr2x(u8 x, u8 val);

    void update_nr4x(u8 x, u8 val);

    void update_lr_enable(u8 lrEnableRegister);

   private:
    Memory* memory;

    // Square 1
    u8* nr10;  // -PPP NSSS Sweep period, negate, shift
    u8* nr11;  // DDLL LLLL Duty, Length load
    u8* nr12;  // VVVV APPP Starting volume, Envelope add mode, period
    u8* nr13;  // FFFF FFFF Frequency LSB
    u8* nr14;  // TL-- -FFF Trigger, Length enable, Frequency MSB

    // Square 2
    u8* nr21;  // DDLL LLLL Duty, Length load
    u8* nr22;  // VVVV APPP Starting volume, Envelope add mode, period
    u8* nr23;  // FFFF FFFF Frequency LSB
    u8* nr24;  // TL-- -FFF Trigger, Length enable, Frequency MSB

    // Master control
    u8* nr50;  // ALLL BRRR Vin L enable, Left vol, Vin R enable, Right vol
    u8* nr51;  // NW21 NW21 Left enables, Right enables
    u8* nr52;  // P--- NW21 Power control, Channel length statuses

    short frameSequenceClocks;
    char frameSequenceStep;
    SampleQueue sampleQueue;

    SquareChannel square1;
    SquareChannel square2;

    NoiseChannel noise;

    u8 leftVol;
    u8 rightVol;
};