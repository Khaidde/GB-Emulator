#include <SDL.h>

#include <cstdio>

#include "game_boy.hpp"

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_VIDEO);

    // GameBoy gameBoy("roms/Tetris.gb");
    GameBoy gameBoy(argv[1]);

    gameBoy.begin();

    SDL_Quit();
    return 0;
}