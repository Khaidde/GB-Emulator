#include "input.hpp"
#include "memory.hpp"

void Input::handle_input(bool pressed, u8 key) {
    u8 keyBit = 1 << key;

    if (!(~keystate & keyBit) && pressed) {
        memory->request_interrupt(Interrupt::JOYPAD_INT);
    }
    keystate = (keystate | keyBit) & ~(keyBit * pressed);
}

u8 Input::get_key_state(bool btnSelect, bool dirSelect) {
    u8 keysReg = 0xC0 | (btnSelect << 5) | (dirSelect << 4);
    if (!btnSelect) {
        keysReg |= keystate >> 4;
    } else if (!dirSelect) {
        keysReg |= keystate & 0xF;
    } else {
        keysReg |= 0xF;
    }
    return keysReg;
}