#include "gba/ppu.h"
#include "types.h"

// GBA color format is BGR555:
// Bits 0-4 = Red (0-31)
// Bits 5-9 = Green (0-31)
// Bits 10-14 = Blue (0-31)
// It scales each channgel from 5-bit (0-31) to 8-bit (0-255)

uint32_t PPU::bgr555_to_rgba(uint16_t color)
{
    uint8_t r = (color & 0x001F) << 3;       // bits 0-4
    uint8_t g = ((color >> 5) & 0x1F) << 3;  // bits 5-9
    uint8_t b = ((color >> 10) & 0x1F) << 3; // bits 10-14
    return (0xFF << 24) | (b << 16) | (g << 8) | r;
}

void PPU::render_frame(const Memory &mem, uint32_t *fb)
{
    // DISPCNT is the first IO register at 0x04000000
    uint16_t dispcnt = mem.read16(0x04000000);

    // Bit 7 = forced blank -- screen is white
    if (dispcnt & (1 << 7))
    {
        for (int i = 0; i < GBA_PIXELS; i++)
            fb[i] = 0xFFFFFFFF;
        return;
    }

    uint8_t mode = dispcnt & 0x7; // bit 0-2

    switch (mode)
    {
    case 3:
        render_mode3(mem, fb);
        break;
    case 4:
        render_mode4(mem, fb);
        break;
    default:
        // Mode 0,1,2,5 -- tiled modes, Sprint 4
        // Fill with black for now
        for (int i = 0; i < GBA_PIXELS; i++)
            fb[i] = 0xFF000000;
        break;
    }
}

void PPU::render_mode3(const Memory &mem, uint32_t *fb)
{
    // Mode 3: 240x160 15-bit bitmap
    // VRAM base: 0x06000000
    // Each pixel is 2 bytes (uint16_t) in BGR format
    // Total: 240 * 160 * 2 = 76800 bytes

    for (int y = 0; y < GBA_H; y++)
    {
        for (int x = 0; x < GBA_W; x++)
        {
            uint32_t addr = 0x06000000 + (y * GBA_W + x) * 2;
            uint16_t color = mem.read16(addr);
            fb[y * GBA_W + x] = bgr555_to_rgba(color);
        }
    }
}

void PPU::render_mode4(const Memory &mem, uint32_t *fb)
{
    // Mode 4: 240x160 paletted bitmap
    // Each pixel is 1 byte = index into palette RAM
    // Palette RAM base: 0x05000000
    // VRAM base: 0x06000000 (page 0) or 0x0600A000 (page 1)

    uint16_t dispcnt = mem.read16(0x04000000);
    uint32_t vram_base = (dispcnt & (1 << 4)) ? 0x0600A000 : 0x06000000;

    for (int y = 0; y < GBA_H; y++)
    {
        for (int x = 0; x < GBA_W; x++)
        {
            uint32_t addr = vram_base + y * GBA_W + x;
            uint8_t index = mem.read8(addr);

            // Look up color in palette RAM (each entry is 2 bytes)
            uint16_t color = mem.read16(0x05000000 + index * 2);
            fb[y * GBA_W + x] = bgr555_to_rgba(color);
        }
    }
}