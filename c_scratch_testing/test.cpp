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

int main(int argc, char** argv) {
    bool c = true;
    u8 reg = 0xEF;
    u8 res = (reg << 1) | c;
    printf("%d", res);

    /*
        set_flag(Z_FLAG, reg == 0);
        set_flag(N_FLAG, false);
        set_flag(H_FLAG, false);
        set_flag(C_FLAG, reg & (1 << 7));   */
    return 0;
}