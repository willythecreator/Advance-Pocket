#pragma once
#include <SDL2/SDL.h>
#include <string>

class Window
{
public:
    Window(const std::string &title, int w, int h);
    ~Window();

    Window(const Window &) = delete;

    Window &operator=(const Window &) = delete;

    bool is_open() const { return open_; }

    SDL_Window *sdl() const { return win_; }

    void swap_buffers();
    void set_title(const std::string &t);

private:
    SDL_Window *win_ = nullptr;
    SDL_GLContext ctx_ = nullptr;
    bool open_ = false;
};