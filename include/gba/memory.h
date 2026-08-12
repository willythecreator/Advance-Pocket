#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <string>

// GBA Memory map (for 32 bits)
//  0x00000000 – 0x00003FFF   BIOS ROM          (16 KB)
//  0x02000000 – 0x0203FFFF   External WRAM     (256 KB)
//  0x03000000 – 0x03007FFF   Internal WRAM     (32 KB)
//  0x04000000 – 0x040003FF   IO Registers
//  0x05000000 – 0x050003FF   Palette RAM       (1 KB)
//  0x06000000 – 0x06017FFF   VRAM              (96 KB)
//  0x07000000 – 0x070003FF   OAM               (1 KB)
//  0x08000000 – 0x09FFFFFF   ROM (GamePak)     (up to 32 MB)

struct Memory
{
    std::array<uint8_t, 0x04000> bios = {};   // 16 KB
    std::array<uint8_t, 0x40000> ewram = {};  // 256 KB
    std::array<uint8_t, 0x08000> iwram = {};  // 32 KB
    std::array<uint8_t, 0x00400> io = {};     // IO registers
    std::array<uint8_t, 0x00400> palram = {}; // Palette RAM
    std::array<uint8_t, 0x18000> vram = {};   // 96 KB VRAM
    std::array<uint8_t, 0x00400> oam = {};    // OAM
    std::vector<uint8_t> rom;                 // GamePak (variable)

    // Bus interface

    uint8_t read8(uint32_t addr) const;
    uint16_t read16(uint32_t addr) const;
    uint32_t read32(uint32_t addr) const;

    void write8(uint32_t addr, uint8_t val);
    void write16(uint32_t addr, uint16_t val);
    void write32(uint32_t addr, uint32_t val);

    bool load_rom(const std::string &path);
    void reset();
};