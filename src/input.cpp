#include "input.hpp"

#include "memory.hpp"

Input::Input(Memory& memory) : memory(&memory) {}

void Input::restart() { keyState = 0xFF; }

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
