#pragma once
#include <cstdint>
#include <string>

// list of possible states
enum class EmuState
{
    IDLE,
    MENU,
    RUNNING,
    PAUSED,
    SWAPPING,
};

// GBA hardware constants

constexpr int GBA_W = 240;
constexpr int GBA_H = 160;
constexpr int GBA_PIXELS = GBA_W * GBA_H;
constexpr int DISPLAY_SCALE = 6; // 6x = 1440x960

constexpr int DISPLAY_W = GBA_W * DISPLAY_SCALE;
constexpr int DISPLAY_H = GBA_H * DISPLAY_SCALE;
constexpr int GBA_FPS = 60;
constexpr uint32_t FRAME_MS = 1000 / GBA_FPS; // ~16ms