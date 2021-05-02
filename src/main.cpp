#include <SDL.h>

#include "game_boy.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: gbemu [romPath].gb");
        return 1;
    }
    SDL_Init(SDL_INIT_VIDEO);

    GameBoy gameBoy(argv[1]);

    gameBoy.begin();

    SDL_Quit();
    return 0;
}