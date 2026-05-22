#include "gba/memory.h"
#include <fstream>
#include <cstring>
#include <cstdio>

static uint8_t *region(Memory &m, uint32_t addr, uint32_t &offset)
{
    addr &= 0x0FFFFFFF; // mirror mask
    if (addr < 0x00004000)
    {
        offset = addr;
        return m.bios.data();
    }
    else if (addr >= 0x02000000 && addr < 0x02040000)
    {
        offset = addr - 0x02000000;
        return m.ewram.data();
    }
    else if (addr >= 0x03000000 && addr < 0x03008000)
    {
        offset = addr - 0x03000000;
        return m.iwram.data();
    }
    else if (addr >= 0x04000000 && addr < 0x04000400)
    {
        offset = addr - 0x04000000;
        return m.io.data();
    }
    else if (addr >= 0x05000000 && addr < 0x05000400)
    {
        offset = addr - 0x05000000;
        return m.palram.data();
    }
    else if (addr >= 0x06000000 && addr < 0x06018000)
    {
        offset = addr - 0x06000000;
        return m.vram.data();
    }
    else if (addr >= 0x07000000 && addr < 0x07000400)
    {
        offset = addr - 0x07000000;
        return m.oam.data();
    }
    else if (addr >= 0x08000000 && !m.rom.empty())
    {
        offset = addr - 0x08000000;
        return m.rom.data();
    }
    offset = 0;
    return nullptr;
}

static const uint8_t *region_c(const Memory &m, uint32_t addr, uint32_t &offset)
{
    return region(const_cast<Memory &>(m), addr, offset);
}

uint8_t Memory::read8(uint32_t addr) const
{
    uint32_t off;
    auto *r = region_c(*this, addr, off);
    return r ? r[off] : 0;
}

uint16_t Memory::read16(uint32_t addr) const
{
    // GBA is little-endian
    return uint16_t(read8(addr)) | (uint16_t(read8(addr + 1)) << 8);
}

uint32_t Memory::read32(uint32_t addr) const
{
    return uint32_t(read16(addr)) | (uint32_t(read16(addr + 2)) << 16);
}

void Memory::write8(uint32_t addr, uint8_t val)
{
    uint32_t off;
    auto *r = region(*this, addr, off);
    if (r)
        r[off] = val;
}

void Memory::write16(uint32_t addr, uint16_t val)
{
    write8(addr, val & 0xFF);
    write8(addr + 1, val >> 8);
}

void Memory::write32(uint32_t addr, uint32_t val)
{
    write16(addr, val & 0xFFFF);
    write16(addr + 2, val >> 16);
}

bool Memory::load_rom(const std::string &path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
    {
        printf("[Memory] Cannot open %s\n", path.c_str());
        return false;
    }

    auto size = f.tellg();
    f.seekg(0);
    rom.resize(size);
    f.read(reinterpret_cast<char *>(rom.data()), size);
    printf("[Memory] ROM loaded: %zu bytes\n", rom.size());
    return true;
}

void Memory::reset()
{
    bios.fill(0);
    ewram.fill(0);
    iwram.fill(0);
    io.fill(0);
    palram.fill(0);
    vram.fill(0);
    oam.fill(0);
    rom.clear();
}