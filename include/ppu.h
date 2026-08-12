#pragma once
#include "memory.h"
#include <cstdint>

struct PPU
{
    // Render a full frame into fb (240*160 RGBA pixels)
    // Call once per frame after CPU has finished executing
    void render_frame(const Memory &mem, uint32_t *fb);

private:
    // Mode 0: 4 regular (text) tiled background layers, no rotation/scaling
    void render_mode0(const Memory &mem, uint32_t *fb);
    void render_bg_layer(const Memory &mem, uint32_t *fb, int bg);

    // Mode 3: raw 16-bit bitmap in VRAM
    void render_mode3(const Memory &mem, uint32_t *fb);

    void render_mode4(const Memory &mem, uint32_t *fb);

    // Convert GBA 15-bit BGR555 color to RGBA8888
    static uint32_t bgr555_to_rgba(uint16_t color);
};
