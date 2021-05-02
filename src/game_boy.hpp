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

   public:
    void begin();

    void render_screen();

   private:
    Debugger debugger;

    CPU cpu;
    Memory memory;

    Cartridge cartridge;
    Input input;
    Timer timer;
    PPU ppu;
    Screen screen;
};