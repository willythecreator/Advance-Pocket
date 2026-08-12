#include "emulator.h"
#include <cstring>
#include <cstdio>

Emulator::Emulator()
{
    std::memset(fb_, 0, sizeof(fb_));
}

void Emulator::open_menu()
{
    if (state_ == EmuState::RUNNING || state_ == EmuState::PAUSED)
    {
        printf("[Emu] → MENU (core frozen, state intact)\n");
        state_ = EmuState::MENU;
    }
}

void Emulator::pause()
{
    if (state_ == EmuState::RUNNING)
    {
        printf("[Emu] → PAUSED\n");
        state_ = EmuState::PAUSED;
    }
}

void Emulator::resume()
{
    if (state_ == EmuState::PAUSED)
    {
        printf("[Emu] → RUNNING\n");
        state_ = EmuState::RUNNING;
    }
}

void Emulator::load_rom(const std::string &path)
{
    printf("[Emu] Loading ROM: %s\n", path.c_str());
    state_ = EmuState::SWAPPING;
    teardown();

    if (!mem_.load_rom(path))
    {
        printf("[Emu] Failed to load ROM — returning to IDLE\n");
        state_ = EmuState::IDLE;
        return;
    }

    rom_path_ = path;
    cpu_.reset();
    printf("[Emu] → RUNNING\n");
    state_ = EmuState::RUNNING;
}

void Emulator::reset()
{
    printf("[Emu] Hard reset — wiping all hardware state\n");
    cpu_.reset();
    mem_.reset();
    std::memset(fb_, 0, sizeof(fb_));
    state_ = EmuState::IDLE;
}

void Emulator::step_frame()
{
    if (state_ != EmuState::RUNNING)
        return;
    constexpr int CYCLES_PER_FRAME = 280896;
    int cycles = 0;
    while (cycles < CYCLES_PER_FRAME)
    {
        cycles += cpu_.step(mem_);
    }

    render_ppu();
}

// Private

void Emulator::teardown()
{
    cpu_.reset();
    mem_.reset();
    std::memset(fb_, 0, sizeof(fb_));
}

void Emulator::render_ppu()
{
    ppu_.render_frame(mem_, fb_);
}

void Emulator::close_menu()
{
    if (state_ == EmuState::MENU)
    {
        printf("[Emu] → RUNNING (resuming from menu)\n");
        state_ = EmuState::RUNNING;
    }
}