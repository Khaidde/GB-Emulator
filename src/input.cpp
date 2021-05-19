#include "input.hpp"
#include "memory.hpp"

void Input::handle_input(bool pressed, u8 key) {
    u8 keyBit = 1 << key;

    if (!(~keyState & keyBit) && pressed) {
        memory->request_interrupt(Interrupt::JOYPAD_INT);
    }
    keyState = (keyState | keyBit) & ~(keyBit * pressed);
}

u8 Input::get_key_state(u8 keyReg) {
    bool btnSelect = (~keyReg >> 5) & 1;
    bool dirSelect = (~keyReg >> 4) & 1;

    u8 upKeyReg = keyReg & 0xF0;
    if (btnSelect) {
        upKeyReg |= keyState >> 4;
    } else if (dirSelect) {
        upKeyReg |= keyState & 0xF;
    } else {
        upKeyReg |= 0xF;
    }
    return upKeyReg;
}
/*
u8 Input::get_key_state(bool btnSelect, bool dirSelect) {
    u8 keysReg = 0xC0 | (!btnSelect << 5) | (!dirSelect << 4);
    if (!btnSelect) {
        keysReg |= keyState >> 4;
    } else if (!dirSelect) {
        keysReg |= keyState & 0xF;
    } else {
        keysReg |= 0xF;
    }
    return keysReg;
}
*/