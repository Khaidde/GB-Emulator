#include "input.hpp"

void Input::handle_input(SDL_Event keyEvent) {
    if (keyEvent.type != SDL_KEYDOWN && keyEvent.type != SDL_KEYUP) return;

    u8 key;
    switch (keyEvent.key.keysym.sym) {
        case SDLK_n:  // START
            key = 7;
            break;
        case SDLK_b:  // SELECT
            key = 6;
            break;
        case SDLK_j:  // B
            key = 5;
            break;
        case SDLK_k:  // A
            key = 4;
            break;

        case SDLK_s:  // DOWN
            key = 3;
            break;
        case SDLK_w:  // UP
            key = 2;
            break;
        case SDLK_a:  // LEFT
            key = 1;
            break;
        case SDLK_d:  // RIGHT
            key = 0;
            break;
        default:
            return;
    }

    u8 keyBit = 1 << key;
    bool isPress = keyEvent.type == SDL_KEYDOWN;

    if (!(~keystate & keyBit) && isPress) {
        memory->request_interrupt(Memory::JOYPAD_INT);
    }
    keystate = (keystate | keyBit) & ~(keyBit * isPress);
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