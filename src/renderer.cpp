#include "renderer.h"
#include "types.h"
#include <SDL2/SDL_opengl.h>
#include <cstring>
#include <cmath>
#include <cstdio>

Renderer::Renderer(int window_w, int window_h)
    : win_w_(window_w), win_h_(window_h)
{
    init_gl();
}

Renderer::~Renderer()
{
    if (texture_)
        glDeleteTextures(1, &texture_);
}

void Renderer::init_gl()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f); // dark background

    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GBA_W, GBA_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    printf("[Renderer] GL texture %u allocated (%dx%d)\n", texture_, GBA_W, GBA_H);
}

void Renderer::draw_frame(const uint32_t *pixels)
{
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GBA_W, GBA_H, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glClear(GL_COLOR_BUFFER_BIT);
    draw_quad();
}

void Renderer::draw_test_pattern()
{
    static int frame = 0;
    frame++;

    uint32_t pixels[GBA_PIXELS];
    for (int y = 0; y < GBA_H; y++)
    {
        for (int x = 0; x < GBA_W; x++)
        {
            int band = (x * 8) / GBA_W;
            uint8_t r = 0, g = 0, b = 0;
            switch (band)
            {
            case 0:
                r = 255;
                g = 0;
                b = 0;
                break; // red
            case 1:
                r = 255;
                g = 128;
                b = 0;
                break; // orange
            case 2:
                r = 255;
                g = 255;
                b = 0;
                break; // yellow
            case 3:
                r = 0;
                g = 255;
                b = 0;
                break; // green
            case 4:
                r = 0;
                g = 255;
                b = 255;
                break; // cyan
            case 5:
                r = 0;
                g = 0;
                b = 255;
                break; // blue
            case 6:
                r = 128;
                g = 0;
                b = 255;
                break; // violet
            case 7:
                r = 255;
                g = 0;
                b = 255;
                break; // magenta
            }

            // Darken bottom half to show gradient
            float brightness = 0.4f + 0.6f * (float(GBA_H - y) / GBA_H);
            r = uint8_t(r * brightness);
            g = uint8_t(g * brightness);
            b = uint8_t(b * brightness);

            // Moving white scanline
            int scanline = (frame * 2) % GBA_H;

            if (y == scanline || y == (scanline + 1) % GBA_H)
                r = g = b = 255;

            pixels[y * GBA_W + x] = (255u << 24) | (b << 16) | (g << 8) | r;
        }
    }
    draw_frame(pixels);
}

void Renderer::draw_quad()
{
    // Integer scaled
    int dst_w = GBA_W * scale_;
    int dst_h = GBA_H * scale_;
    int dst_x = (win_w_ - dst_w) / 2;
    int dst_y = (win_h_ - dst_h) / 2;

    // Map pixel coords to OpenGL
    float x0 = (2.0f * dst_x / win_w_) - 1.0f;
    float x1 = (2.0f * (dst_x + dst_w) / win_w_) - 1.0f;
    float y0 = 1.0f - (2.0f * dst_y / win_h_);
    float y1 = 1.0f - (2.0f * (dst_y + dst_h) / win_h_);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_);

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0, 0);
    glVertex2f(x0, y0);
    glTexCoord2f(1, 0);
    glVertex2f(x1, y0);
    glTexCoord2f(0, 1);
    glVertex2f(x0, y1);
    glTexCoord2f(1, 1);
    glVertex2f(x1, y1);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void Renderer::set_scale(int scale)
{
    scale_ = scale;
}