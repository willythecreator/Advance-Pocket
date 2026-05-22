#include "window.h"
#include "renderer.h"
#include "emulator.h"
#include "types.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <string>

// Frame timing

int main(int argc, char *argv[])
{
    printf("Advance Pocket v0.1 - SDL2 + OpenGL renderer\n");

    Window window("Advance Pocket", DISPLAY_W, DISPLAY_H);
    Renderer renderer(DISPLAY_W, DISPLAY_H);
    Emulator emu;

    // Load ROM from command line if provided

    if (argc > 1)
    {
        emu.load_rom(argv[1]);
    }

    bool running = true;
    uint32_t frame_start = 0;

    while (running && window.is_open())
    {
        frame_start = SDL_GetTicks();

        // Event handling

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                switch (e.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                    // ESC: open/close menu
                    if (emu.state() == EmuState::MENU)
                        emu.close_menu();
                    else
                        emu.open_menu();
                    break;

                case SDLK_SPACE:
                    // SPACE: pause / resume
                    if (emu.state() == EmuState::RUNNING)
                        emu.pause();

                    else if (emu.state() == EmuState::PAUSED)
                        emu.resume();
                    break;

                case SDLK_r:
                    // R: hard reset
                    emu.reset();
                    break;

                case SDLK_t:
                    // T: toggle test pattern (useful before a ROM is loaded)
                    printf("[Main] Showing test pattern\n");
                    break;

                default:
                    break;
                }
                break;

            case SDL_DROPFILE:
            {
                // Drag a .gba file onto the window to load it
                std::string path(e.drop.file);
                SDL_free(e.drop.file);
                printf("[Main] Dropped file: %s\n", path.c_str());
                emu.load_rom(path);
                break;
            }

            default:
                break;
            }
        }

        // Update
        emu.step_frame(); // no-op unless RUNNING

        // Render
        if (emu.state() == EmuState::IDLE || emu.state() == EmuState::MENU)
        {
            // No ROM loaded - show animated test pattern so the screen isn't black
            renderer.draw_test_pattern();
        }
        else
        {
            renderer.draw_frame(emu.framebuffer());
        }

        window.swap_buffers();

        // Frame cap
        uint32_t elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < FRAME_MS)
            SDL_Delay(FRAME_MS - elapsed);
    }

    printf("[Main] Shutting down\n");
    return 0;
}