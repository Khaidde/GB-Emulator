#include <SDL.h>

#include "game_boy.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: gbemu [romPath].gb");
        return 1;
    }
    SDL_Init(SDL_INIT_VIDEO);

    try {
        GameBoy gameBoy(argv[1]);

        gameBoy.begin();
    } catch (const std::exception& e) {
        fprintf(stderr, e.what());
        SDL_Quit();
        return 1;
    }

    SDL_Quit();
    return 0;
}