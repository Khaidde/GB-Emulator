#pragma once

#include <SDL.h>

#include "cartridge.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "ppu.hpp"

struct Screen {
    static constexpr int PIXEL_SCALE = 4;

    SDL_Window* window;
    SDL_Renderer* renderer;

    int pitch = 0;
    SDL_Texture* bufferTexture;
};

class GameBoy {
   public:
    static constexpr const char* TITLE = "GameBoy Emulator";
    static constexpr int WIDTH = 160;
    static constexpr int HEIGHT = 144;

    GameBoy(const char*);
    ~GameBoy();

   public:
    void begin();

    void render_screen();

    void handle_input(SDL_Event keyEvent);
    u8 get_key_state();

   private:
    CPU cpu;
    Cartridge cartridge;
    Memory memory;
    PPU ppu;

    Screen screen;
    u8 keyState;
};