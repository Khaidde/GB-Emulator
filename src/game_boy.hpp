#pragma once

#include <SDL.h>

#include "cartridge.hpp"
#include "cpu.hpp"
#include "debugger.hpp"
#include "input.hpp"
#include "memory.hpp"
#include "ppu.hpp"
#include "timer.hpp"

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
    void begin();
    char get_gb_key(SDL_Keycode keycode);

    void render_screen();

   private:
    Screen screen;

    Debugger debugger;

    Input input;
    Timer timer;
    PPU ppu;
    Memory memory;
    CPU cpu;
};