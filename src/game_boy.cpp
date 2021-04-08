#include "game_boy.hpp"

#include <cstdio>
#include <chrono>
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

GameBoy::GameBoy(const char* romPath) : memory(), cpu(&memory), ppu(&memory), cartridge(romPath) {
    memory.startup();
    cpu.startup();
    memory.load_cartridge(&cartridge);
    keyState = 0xFF;

    create_screen(screen, GameBoy::WIDTH * Screen::PIXEL_SCALE, GameBoy::HEIGHT * Screen::PIXEL_SCALE, GameBoy::TITLE);
}

static void destroy_screen(Screen& screen) {
    SDL_DestroyTexture(screen.bufferTexture);
    SDL_DestroyRenderer(screen.renderer);
    SDL_DestroyWindow(screen.window);
};

GameBoy::~GameBoy() { destroy_screen(screen); }

void GameBoy::begin() {
    using namespace std::chrono_literals;

    std::chrono::high_resolution_clock timer;
    std::chrono::nanoseconds delta(0ns);
    auto last = timer.now();
    static constexpr std::chrono::nanoseconds FRAME_MS(16742000ns);  // Roughly 60fps

    SDL_Event e;
    bool running = true;

    int mcycleCount = 0;
    int other = 0;

    bool space = false;
    bool stepped = false;
    while (running) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    handle_input(e);
                    if (e.key.keysym.sym == SDLK_SPACE) {
                        if (e.type == SDL_KEYDOWN) {
                            if (!space) {
                                space = true;
                                stepped = false;
                            }
                        } else {
                            space = false;
                        }
                    }
                    break;
            }
        }

        // TODO handle time accurate clock cycles
        while (mcycleCount < PPU::TOTAL_CYCLES) {
            if (!cpu.stepMode) stepped = false;
            if (stepped) break;
            stepped = true;

            u8 deltaCycles = 0;
            cpu.tick(deltaCycles);
            ppu.proccess(deltaCycles);
            mcycleCount += deltaCycles;
        }
        render_screen();

        auto now = timer.now();
        delta += std::chrono::duration_cast<std::chrono::nanoseconds>(now - last);
        last = now;
        while (delta >= FRAME_MS) {
            mcycleCount = 0;
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

void GameBoy::handle_input(SDL_Event keyEvent) {
    if (keyEvent.type != SDL_KEYDOWN && keyEvent.type != SDL_KEYUP) return;

    u8 key;
    switch (keyEvent.key.keysym.sym) {
        case SDLK_RETURN:  // START
            key = 7;
            break;
        case SDLK_SPACE:  // SELECT
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
    if (!(~keyState & keyBit) && isPress) {
        memory.request_interrupt(Memory::JOYPAD_INT);
    }
    keyState = (keyState | keyBit) & ~(keyBit * isPress);
}

u8 GameBoy::get_key_state() {
    u8 joypadReg = memory.read(Memory::JOYP_REG);

    static constexpr u8 BTN_SELECT = (1 << 5);
    static constexpr u8 DIR_SELECT = (1 << 4);
    if (~joypadReg & BTN_SELECT) {
        joypadReg = (joypadReg & 0xF0) | (keyState >> 4);
    } else if (~joypadReg & DIR_SELECT) {
        joypadReg = (joypadReg & 0xF0) | (keyState & 0xF);
    }
    return joypadReg;
}