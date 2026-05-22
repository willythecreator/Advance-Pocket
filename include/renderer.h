#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <cstdint>

class Renderer
{
public:
    Renderer(int window_w, int window_h);
    ~Renderer();

    void draw_frame(const uint32_t *pixels);

    void draw_test_pattern();

    void set_scale(int scale);

private:
    void init_gl();
    void draw_quad();

    GLuint texture_ = 0;
    int win_w_ = 0;
    int win_h_ = 0;
    int scale_ = 6;
};