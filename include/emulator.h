#pragma once
#include "types.h"
#include "gba/cpu.h"
#include "gba/memory.h"
#include "gba/ppu.h"
#include <string>
#include <cstdint>

class Emulator
{
public:
    Emulator();
    ~Emulator() = default;

    // State machine
    EmuState state() const { return state_; }

    void open_menu();
    void close_menu();
    void pause();
    void resume();
    void load_rom(const std::string &path);
    void reset();

    // per frame update
    void step_frame();

    const uint32_t *framebuffer() const { return fb_; }

private:
    EmuState state_ = EmuState::IDLE;
    CPU cpu_;
    Memory mem_;
    PPU ppu_;
    uint32_t fb_[GBA_PIXELS] = {};

    std::string rom_path_;

    void teardown();

    void render_ppu();
};