#include "game_boy.hpp"

#include <cstdio>
#include <chrono>

namespace {

void create_screen(Screen& screen, int width, int height, const char* title) {
    screen.window =
        SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);

    if (!screen.window) {
        std::fprintf(stderr, "Window could not be created! SDL_error: %s\n", SDL_GetError());
    }
    screen.renderer = SDL_CreateRenderer(screen.window, -1, SDL_RENDERER_SOFTWARE);
    screen.bufferTexture = SDL_CreateTexture(screen.renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING,
                                             GameBoy::WIDTH, GameBoy::HEIGHT);
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

}  // namespace

GameBoy::GameBoy(const char* romPath) {
    debugger.init(&cpu, &memory);

    cpu.init(&memory, &debugger);

    memory.set_debugger(&debugger);
    memory.load_cartridge(romPath);

    input.init(&memory);
    timer.init(&memory);
    ppu.init(&memory);
    memory.set_peripherals(&input, &timer, &ppu);

    create_screen(screen, GameBoy::WIDTH * Screen::PIXEL_SCALE, GameBoy::HEIGHT * Screen::PIXEL_SCALE, GameBoy::TITLE);
}

GameBoy::~GameBoy() { destroy_screen(screen); }

void GameBoy::begin() {
    using namespace std::chrono_literals;

    std::chrono::high_resolution_clock sysTimer;
    std::chrono::nanoseconds delta(0ns);
    auto last = sysTimer.now();
    static constexpr std::chrono::nanoseconds FRAME_MS(16742000ns);  // Roughly 60fps

    SDL_Event e;
    bool running = true;

    int totalMCycles = 0;

    while (running) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYUP: {
                    char key = get_gb_key(e.key.keysym.sym);
                    debugger.handle_function_key(e);
                    if (key >= 0) input.handle_input(false, key);
                } break;
                case SDL_KEYDOWN: {
                    char key = get_gb_key(e.key.keysym.sym);
                    if (key >= 0) input.handle_input(true, key);
                } break;
            }
            break;
        }

        while (totalMCycles < (int)PPU::TOTAL_CLOCKS / 4) {
            if (cpu.isFetching() && debugger.is_paused() && !debugger.step()) break;

            for (int i = 0; i < 4; i++) {
                timer.emulate_clock();
                ppu.emulate_clock();
            }
            cpu.emulate_cycle();
            totalMCycles++;
        }
        render_screen();

        auto now = sysTimer.now();
        delta += std::chrono::duration_cast<std::chrono::nanoseconds>(now - last);
        last = now;
        if (delta >= FRAME_MS) {
            totalMCycles -= (int)PPU::TOTAL_CLOCKS / 4;
            delta -= FRAME_MS;
        }
    }
}

char GameBoy::get_gb_key(SDL_Keycode keycode) {
    switch (keycode) {
        case SDLK_n:  // START
            return 7;
        case SDLK_b:  // SELECT
            return 6;
        case SDLK_j:  // B
            return 5;
        case SDLK_k:  // A
            return 4;

        case SDLK_s:  // DOWN
            return 3;
        case SDLK_w:  // UP
            return 2;
        case SDLK_a:  // LEFT
            return 1;
        case SDLK_d:  // RIGHT
            return 0;

        default:
            return -1;
    }
}

void GameBoy::render_screen() {
    SDL_RenderClear(screen.renderer);

    u32* pixelBuffer;
    if (SDL_LockTexture(screen.bufferTexture, nullptr, (void**)&pixelBuffer, &screen.pitch)) {
        fatal("Failed to lock texture! SDL_error: %s\n", SDL_GetError());
    }
    screen.pitch /= sizeof(u32);

    ppu.render(pixelBuffer);

    SDL_UnlockTexture(screen.bufferTexture);

    SDL_RenderCopy(screen.renderer, screen.bufferTexture, NULL, NULL);
    SDL_RenderPresent(screen.renderer);
}