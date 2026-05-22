#include "window.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdexcept>
#include <cstdio>

Window::Window(const std::string &title, int w, int h)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
        throw std::runtime_error(SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0); // no depth buffer needed

    win_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!win_)
        throw std::runtime_error(SDL_GetError());

    ctx_ = SDL_GL_CreateContext(win_);
    if (!ctx_)
        throw std::runtime_error(SDL_GetError());

    SDL_GL_SetSwapInterval(1);

    printf("[Window] %dx%d OpenGL %s\n", w, h, glGetString(GL_VERSION));
    open_ = true;
}

Window::~Window()
{
    if (ctx_)
        SDL_GL_DeleteContext(ctx_);
    if (win_)
        SDL_DestroyWindow(win_);
    SDL_Quit();
}

void Window::swap_buffers()
{
    SDL_GL_SwapWindow(win_);
}

void Window::set_title(const std::string &t)
{
    SDL_SetWindowTitle(win_, t.c_str());
}