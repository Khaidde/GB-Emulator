#include <SDL.h>

#include "game_boy.hpp"

namespace {

struct Screen {
    static constexpr int PIXEL_SCALE = 4;

    SDL_Window* window;
    SDL_Renderer* renderer;

    int pitch = 0;
    SDL_Texture* bufferTexture;
};

void create_screen(Screen& screen, int width, int height, const char* title) {
    screen.window =
        SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);

    if (!screen.window) {
        std::fprintf(stderr, "Window could not be created! SDL_error: %s\n", SDL_GetError());
    }
    screen.renderer = SDL_CreateRenderer(screen.window, -1, SDL_RENDERER_SOFTWARE);
    screen.bufferTexture = SDL_CreateTexture(screen.renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING,
                                             Constants::WIDTH, Constants::HEIGHT);
    if (!screen.bufferTexture) {
        std::fprintf(stderr, "Failed to create back buffer texture! SDL_error: %s\n", SDL_GetError());
    }

    screen.pitch = 0;
}

void destroy_screen(Screen& screen) {
    SDL_DestroyTexture(screen.bufferTexture);
    SDL_DestroyRenderer(screen.renderer);
    SDL_DestroyWindow(screen.window);
}

JoypadButton get_gb_keycode(SDL_Keycode keycode) {
    switch (keycode) {
        case SDLK_n:
            return JoypadButton::START_BTN;
        case SDLK_b:
            return JoypadButton::SELECT_BTN;
        case SDLK_j:
            return JoypadButton::B_BTN;
        case SDLK_k:
            return JoypadButton::A_BTN;
        case SDLK_s:
            return JoypadButton::DOWN_BTN;
        case SDLK_w:
            return JoypadButton::UP_BTN;
        case SDLK_a:
            return JoypadButton::LEFT_BTN;
        case SDLK_d:
            return JoypadButton::RIGHT_BTN;
        default:
            return JoypadButton::NONE;
    }
}

void wait_frame() {
    // TODO wait roughly 1/60th of a second
    SDL_Delay(10);
}

void run(Screen& screen, GameBoy& gameboy, Debugger& debugger) {
    SDL_Event e;
    bool running = true;
    while (running) {
        SDL_RenderClear(screen.renderer);

        u32* pixelBuffer;
        if (SDL_LockTexture(screen.bufferTexture, nullptr, (void**)&pixelBuffer, &screen.pitch)) {
            fatal("Failed to lock texture! SDL_error: %s\n", SDL_GetError());
        }
        screen.pitch /= sizeof(u32);
        gameboy.emulate_frame(pixelBuffer);

        SDL_UnlockTexture(screen.bufferTexture);

        SDL_RenderCopy(screen.renderer, screen.bufferTexture, nullptr, nullptr);
        SDL_RenderPresent(screen.renderer);

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYUP:
                    if (e.key.keysym.sym == SDLK_F7) {
                        debugger.step();
                    } else if (e.key.keysym.sym == SDLK_F9) {
                        debugger.continue_exec();
                    } else {
                        JoypadButton key = get_gb_keycode(e.key.keysym.sym);
                        if (key != JoypadButton::NONE) gameboy.handle_key_code(false, key);
                    }
                    break;
                case SDL_KEYDOWN: {
                    JoypadButton key = get_gb_keycode(e.key.keysym.sym);
                    if (key != JoypadButton::NONE) gameboy.handle_key_code(true, key);
                } break;
            }
            break;
        }

        wait_frame();
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: gbemu [romPath.gb]");
        return 1;
    }
    SDL_Init(SDL_INIT_VIDEO);

    GameBoy gameboy;

    Debugger debugger;
    gameboy.set_debugger(debugger);

    Screen screen;
    create_screen(screen, Constants::WIDTH * Screen::PIXEL_SCALE, Constants::HEIGHT * Screen::PIXEL_SCALE,
                  Constants::TITLE);

    try {
        gameboy.load(argv[1]);
        run(screen, gameboy, debugger);
    } catch (const std::exception& e) {
        fprintf(stderr, "%s", e.what());
        SDL_Quit();
        return 1;
    }

    destroy_screen(screen);

    SDL_Quit();
    return 0;
}