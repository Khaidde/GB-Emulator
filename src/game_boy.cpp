#include "game_boy.hpp"

#include <cstdio>
#include <chrono>

static void create_screen(Screen& screen, int width, int height, const char* title) {
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

GameBoy::GameBoy(const char* romPath) {
    debugger.init(&cpu, &memory);

    cpu.init(&memory, &debugger);
    memory.init(&debugger);

    cartridge.load_from_path(romPath);
    ppu.init(&memory);
    input.init(&memory);
    timer.init(&memory);
    memory.load_peripherals(&cartridge, &input, &timer);

    create_screen(screen, GameBoy::WIDTH * Screen::PIXEL_SCALE, GameBoy::HEIGHT * Screen::PIXEL_SCALE, GameBoy::TITLE);
}

static void destroy_screen(Screen& screen) {
    SDL_DestroyTexture(screen.bufferTexture);
    SDL_DestroyRenderer(screen.renderer);
    SDL_DestroyWindow(screen.window);
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
                case SDL_KEYUP:
                    debugger.handle_function_key(e);
                case SDL_KEYDOWN:
                    input.handle_input(e);
            }
            break;
        }

        while (totalMCycles < (int)PPU::TOTAL_CLOCKS / 4) {
            if (debugger.is_paused() && !debugger.step()) break;

            u8 mCycles = cpu.tick();
            timer.tick(mCycles);
            for (int i = 0; i < mCycles * 4; i++) {
                ppu.emulate_clock();
            }
            totalMCycles += mCycles;
        }
        render_screen();

        auto now = sysTimer.now();
        delta += std::chrono::duration_cast<std::chrono::nanoseconds>(now - last);
        last = now;
        while (delta >= FRAME_MS) {
            totalMCycles = 0;
            delta -= FRAME_MS;
        }
    }
}

void GameBoy::render_screen() {
    SDL_RenderClear(screen.renderer);

    u32* pixelBuffer;
    if (SDL_LockTexture(screen.bufferTexture, nullptr, (void**)&pixelBuffer, &screen.pitch)) {
        fprintf(stderr, "Failed to lock texture! SDL_error: %s\n", SDL_GetError());
        return;
    }
    screen.pitch /= sizeof(u32);

    ppu.render(pixelBuffer);

    SDL_UnlockTexture(screen.bufferTexture);

    SDL_RenderCopy(screen.renderer, screen.bufferTexture, NULL, NULL);
    SDL_RenderPresent(screen.renderer);
}
