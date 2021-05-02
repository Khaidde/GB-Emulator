#include <stdint.h>
#include <stdio.h>

using u16 = uint16_t;

static u16 h1;
static u16 c1;
static void add1(u16 hl, u16 val) {
    u16 res = hl + val;
    h1 = (0xFFF - (hl & 0xFFF)) < (val & 0xFFF);
    c1 = (0xFFFF - hl) < val;
}

static u16 h2;
static u16 c2;
static void add2(u16 hl, u16 val) {
    u16 res = hl + val;
    h2 = (hl & 0x0FFF) + (val & 0x0FFF) > 0x0FFF;
    c2 = (hl + val) > 0xFFFF;
}

/*
int main(int argc, char** argv) {
    for (u16 hl = 0; hl <= 0xFFFF; hl++) {
        for (u16 val = 0; val <= 0xFFFF; val++) {
            add1(hl, val);
            add2(hl, val);
            if (h1 != h2 || c1 != c2) {
                printf("%d::%d", hl, val);
                return 0;
            }
            if (val == 0xFFFF) break;
        }
        printf("%d\n", hl);
        if (hl == 0xFFFF) break;
    }
    printf("Hello World!");
    return 0;
}
*/
using u8 = uint8_t;
using s8 = int8_t;

// void doStuff(u8& thing) { thing = 3; }

static const u8 len = 10;
static u8 arrayTest[len];

u8& ref(u8 addr) { return arrayTest[addr]; }

struct FIFO {
    static constexpr u8 FIFO_SIZE = 16;
    u8 headPtr = 0;
    u8 tailPtr = 0;
    u8 size = 0;
    u8 fifo[FIFO_SIZE];

    void push_pixel(u8 pixel) {
        fifo[headPtr] = pixel;
        headPtr = (headPtr + 1) % FIFO_SIZE;
        size++;
    }
    u8 pop_pixel() {
        u8 pixel = fifo[tailPtr];
        tailPtr = (tailPtr + 1) % FIFO_SIZE;
        size--;
        return pixel;
    }
    void flush() {
        headPtr = 0;
        tailPtr = 0;
        size = 0;
    }
};

int main(int argc, char** argv) {
    FIFO testFifo;
    for (int i = 0; i < 12; i++) {
        testFifo.push_pixel(i * 2);
    }
    for (int i = 0; i < 8; i++) {
        printf("%d\n", testFifo.pop_pixel());
    }
    for (int i = 0; i < 12; i++) {
        testFifo.push_pixel(i * 10);
    }
    printf("size=%d\n", testFifo.size);
    testFifo.flush();

    for (int i = 0; i < 12; i++) {
        testFifo.push_pixel(i * 10);
    }

    for (int i = 0; i < 8; i++) {
        printf("%d\n", testFifo.pop_pixel());
    }
    printf("size=%d\n", testFifo.size);

    /*
        set_flag(Z_FLAG, reg == 0);
        set_flag(N_FLAG, false);
        set_flag(H_FLAG, false);
        set_flag(C_FLAG, reg & (1 << 7));   */
    return 0;
}