#pragma once
#include <cstdint>
#include <array>

struct CPU
{
    std::array<uint32_t, 16> r = {}; // r[15] = PC

    uint32_t cpsr = 0; // N Z C V | IRQ | T | mode

    uint32_t &pc() { return r[15]; }
    uint32_t &sp() { return r[13]; }
    uint32_t &lr() { return r[14]; }

    bool thumb_mode() const { return (cpsr >> 5) & 1; }
    bool flag_n() const { return (cpsr >> 31) & 1; }
    bool flag_z() const { return (cpsr >> 30) & 1; }
    bool flag_c() const { return (cpsr >> 29) & 1; }
    bool flag_v() const { return (cpsr >> 28) & 1; }

    // Cycle
    int step(struct Memory &mem);
    void reset();

private:
    bool check_condition(uint8_t cond);
    int execute_arm(uint32_t opcode, Memory &mem);
    int execute_thumb(uint16_t opcode, Memory &mem);

    // ARM operations
    int op_data(uint32_t opcode, Memory &mem);
    int op_ldr_str(uint32_t opcode, Memory &mem);
    int op_branch(uint32_t opcode, Memory &mem);
    int op_bx(uint32_t opcode, Memory &mem);
};