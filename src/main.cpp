#include <SDL.h>

#include "game_boy.hpp"

namespace {

struct Screen {
    static constexpr int PIXEL_SCALE = 4;

    SDL_Window* window;
    SDL_Renderer* renderer;

    int pitch = 0;
    u32* pixelBuffer;
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

JoypadButton get_gb_joybuttoncode(u8 button) {
    switch (button) {
        case 0:
            return JoypadButton::B_BTN;
        case 1:
            return JoypadButton::A_BTN;
        case 6:
            return JoypadButton::SELECT_BTN;
        case 7:
            return JoypadButton::START_BTN;
        default:
            return JoypadButton::NONE;
    }
}

SDL_Joystick* gameController;
constexpr s16 JOYSTICK_DEAD_ZONE = 4000;

constexpr u16 SAMPLE_SIZE = 2048;
constexpr double SAMPLES_PER_FRAME = Constants::SAMPLE_RATE / (1000.0 / Constants::MS_PER_FRAME);
constexpr double FLOATING_OFF = SAMPLES_PER_FRAME - (int)SAMPLES_PER_FRAME;
double frameAcc = 0;

void run(Screen& screen, GameBoy& gameboy, Debugger& debugger) {
    SDL_Event e;
    bool running = true;

    s16 sampleBuffer[SAMPLE_SIZE];

    double nextFrame = SDL_GetTicks();
    while (running) {
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
                case SDL_JOYBUTTONUP: {
                    JoypadButton key = get_gb_joybuttoncode(e.jbutton.button);
                    if (key != JoypadButton::NONE) gameboy.handle_key_code(false, key);
                } break;
                case SDL_JOYBUTTONDOWN: {
                    JoypadButton key = get_gb_joybuttoncode(e.jbutton.button);
                    if (key != JoypadButton::NONE) gameboy.handle_key_code(true, key);
                } break;
                case SDL_JOYHATMOTION:
                    gameboy.handle_key_code((e.jhat.value & 0x1) != 0, JoypadButton::UP_BTN);
                    gameboy.handle_key_code((e.jhat.value & 0x2) != 0, JoypadButton::RIGHT_BTN);
                    gameboy.handle_key_code((e.jhat.value & 0x4) != 0, JoypadButton::DOWN_BTN);
                    gameboy.handle_key_code((e.jhat.value & 0x8) != 0, JoypadButton::LEFT_BTN);
                    break;
                case SDL_JOYAXISMOTION:
                    if (e.jaxis.axis == 0) {
                        if (e.jaxis.value < -JOYSTICK_DEAD_ZONE) {
                            gameboy.handle_key_code(true, JoypadButton::LEFT_BTN);
                            gameboy.handle_key_code(false, JoypadButton::RIGHT_BTN);
                        } else if (e.jaxis.value > JOYSTICK_DEAD_ZONE) {
                            gameboy.handle_key_code(true, JoypadButton::RIGHT_BTN);
                            gameboy.handle_key_code(false, JoypadButton::LEFT_BTN);
                        } else {
                            gameboy.handle_key_code(false, JoypadButton::LEFT_BTN);
                            gameboy.handle_key_code(false, JoypadButton::RIGHT_BTN);
                        }
                    } else {
                        if (e.jaxis.value < -JOYSTICK_DEAD_ZONE) {
                            gameboy.handle_key_code(true, JoypadButton::UP_BTN);
                            gameboy.handle_key_code(false, JoypadButton::DOWN_BTN);
                        } else if (e.jaxis.value > JOYSTICK_DEAD_ZONE) {
                            gameboy.handle_key_code(true, JoypadButton::DOWN_BTN);
                            gameboy.handle_key_code(false, JoypadButton::UP_BTN);
                        } else {
                            gameboy.handle_key_code(false, JoypadButton::UP_BTN);
                            gameboy.handle_key_code(false, JoypadButton::DOWN_BTN);
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        SDL_RenderClear(screen.renderer);

        if (SDL_LockTexture(screen.bufferTexture, nullptr, (void**)&screen.pixelBuffer, &screen.pitch)) {
            fatal("Failed to lock texture! SDL_error: %s\n", SDL_GetError());
        }
        screen.pitch /= sizeof(u32);

        frameAcc += FLOATING_OFF;
        int inc = 0;
        if (frameAcc >= 1) {
            inc = 1;
        }
        int sampleLen = ((int)SAMPLES_PER_FRAME + inc) * 2;
        gameboy.emulate_frame(screen.pixelBuffer, sampleBuffer, sampleLen);
        SDL_QueueAudio(1, sampleBuffer, sampleLen * sizeof(u16));
        frameAcc -= inc;

        SDL_UnlockTexture(screen.bufferTexture);

        SDL_RenderCopy(screen.renderer, screen.bufferTexture, nullptr, nullptr);
        SDL_RenderPresent(screen.renderer);

        nextFrame += Constants::MS_PER_FRAME;
        double delay = nextFrame - SDL_GetTicks();
        if (delay > 0) {
            SDL_Delay(delay);
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: gbemu [romPath.gb]");
        return 1;
    }
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);

    if (SDL_NumJoysticks() > 0) {
        gameController = SDL_JoystickOpen(0);
        if (gameController == nullptr) {
            fprintf(stderr, "Could not open game controller\n");
        }
    } else {
        gameController = nullptr;
    }

    SDL_AudioSpec spec;
    spec.freq = Constants::SAMPLE_RATE;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = SAMPLE_SIZE;
    spec.callback = nullptr;

    if (SDL_OpenAudio(&spec, 0) != 0) {
        fprintf(stderr, "Could not open audio\n");
    }
    SDL_PauseAudio(0);

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
    SDL_CloseAudio();

    SDL_JoystickClose(gameController);
    gameController = nullptr;

    SDL_Quit();
    return 0;
}