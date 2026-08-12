#include "gba/cpu.h"
#include "gba/memory.h"
#include <cstring>

void CPU::reset()
{
    r.fill(0);
    pc() = 0x08000000;
    sp() = 0x03007F00; // end of IWRAM
    cpsr = 0x0000001F; // supervisor mode, ARM state, IRQ+FIQ disabled
}

uint32_t CPU::read_reg(uint8_t idx) const
{
    if (idx == 15)
        return r[15] + 4; // r[15] is already +4 from fetch; add 4 more to reach the real hardware value of instr_addr+8
    return r[idx];
}

int CPU::step(Memory &mem)
{
    static uint64_t counter = 0;
    counter++;

    uint32_t current_pc = pc();
    bool detailed = counter <= 30; // log first 30 instructions in full detail

    if (detailed || counter % 200000 == 0)
    {
        uint32_t raw_opcode = thumb_mode() ? mem.read16(current_pc) : mem.read32(current_pc);
        printf("[%llu] PC=%08X OPCODE=%08X CPSR=%08X mode=%s\n",
               (unsigned long long)counter, current_pc, raw_opcode, cpsr,
               thumb_mode() ? "THUMB" : "ARM");
    }

    if (thumb_mode())
    {
        uint16_t opcode = mem.read16(pc());
        pc() += 2;
        return execute_thumb(opcode, mem);
    }
    else
    {
        uint32_t opcode = mem.read32(pc());
        pc() += 4;
        if (!check_condition(opcode >> 28))
            return 1;
        return execute_arm(opcode, mem);
    }
}

bool CPU::check_condition(uint8_t cond)
{
    switch (cond)
    {
    case 0x0:
        return flag_z(); // EQ
    case 0x1:
        return !flag_z(); // NE
    case 0x2:
        return flag_c(); // CS / HS
    case 0x3:
        return !flag_c(); // CC / LO
    case 0x4:
        return flag_n(); // MI
    case 0x5:
        return !flag_n(); // PL
    case 0x6:
        return flag_v(); // VS
    case 0x7:
        return !flag_v(); // VC
    case 0x8:
        return flag_c() && !flag_z(); // HI
    case 0x9:
        return !flag_c() || flag_z(); // LS
    case 0xA:
        return flag_n() == flag_v(); // GE
    case 0xB:
        return flag_n() != flag_v(); // LT
    case 0xC:
        return !flag_z() && (flag_n() == flag_v()); // GT
    case 0xD:
        return flag_z() || (flag_n() != flag_v()); // LE
    case 0xE:
        return true; // AL
    case 0xF:
        return false; // NV
    default:
        return false;
    }
}

int CPU::execute_arm(uint32_t opcode, Memory &mem)
{
    uint8_t category = (opcode >> 26) & 0x3;

    switch (category)
    {
    case 0b00:
        if ((opcode & 0x0FFFFFF0) == 0x012FFF10)
            return op_bx(opcode, mem);
        return op_data(opcode, mem);

    case 0b01:
        return op_ldr_str(opcode, mem);

    case 0b10: // ← this was missing!
        return op_branch(opcode, mem);

    case 0b11:
        return 1; // SWI / coprocessor for now

    default:
        return 1;
    }
}

int CPU::op_branch(uint32_t opcode, Memory &mem)
{
    bool link = (opcode >> 24) & 1;

    int32_t offset = opcode & 0x00FFFFFF;
    if (offset & 0x00800000)
        offset |= 0xFF000000;
    offset <<= 2;

    if (link)
        lr() = pc(); // already points to next instruction

    // Correct pipeline adjustment
    pc() += offset + 4;

    return 3;
}

int CPU::op_bx(uint32_t opcode, Memory &mem)
{
    uint8_t rn = opcode & 0xF;
    uint32_t target = r[rn];

    // Bit - of target = switch to thumb mode
    if (target & 1)
    {
        cpsr |= (1 << 5);    // set thumb bit
        pc() = target & ~1u; // clear bit 0
    }
    else
    {
        cpsr &= ~(1 << 5);  // clear thumb bit
        pc() = target & ~3; // word align
    }
    return 3;
}

// The big one (I needed 3 packs of Monster Energy)
int CPU::op_data(uint32_t opcode, Memory &mem)
{
    bool imm = (opcode >> 25) & 1;     // is operand 2 an immediate ????
    uint8_t op = (opcode >> 21) & 0xF; // which operation
    bool set_cc = (opcode >> 20) & 1;  // update flags ??
    uint8_t rn = (opcode >> 16) & 0xF; // first operand register
    uint8_t rd = (opcode >> 12) & 0xF; // destination register

    // Decode operand 2
    uint32_t op2 = 0;
    uint32_t carry = flag_c();

    if (imm)
    {
        // Immediate: 8-bit value rotated right by 2 * rotate field
        uint8_t rot = (opcode >> 8) & 0xF;
        uint32_t val = opcode & 0xFF;
        op2 = (val >> (rot * 2)) | (val << (32 - rot * 2));
    }
    else
    {
        // Register: shift r[rm] by shift amount
        uint8_t rm = opcode & 0xF;
        uint8_t stype = (opcode >> 5) & 0x3;
        uint8_t samt = (opcode >> 7) & 0x1F;
        uint32_t base = read_reg(rm);

        switch (stype)
        {
        case 0:
            op2 = base << samt;
            break; // LSL
        case 1:
            op2 = samt ? base >> samt : 0;
            break; // LSR
        case 2:
            op2 = (int32_t)base >> samt;
            break; // ASR
        case 3:
            op2 = samt // ROR
                      ? (base >> samt) | (base << (32 - samt))
                      : (carry << 31) | (base >> 1);
            break; // RRX
        }
    }

    uint32_t a = read_reg(rn);
    uint64_t result = 0;

    // Execute operation
    switch (op)
    {
    case 0x0:
        result = a & op2;
        break; // AND
    case 0x1:
        result = a ^ op2;
        break; // EOR
    case 0x2:
        result = a - op2;
        break; // SUB
    case 0x3:
        result = op2 - a;
        break; // RSB
    case 0x4:
        result = a + op2;
        break; // ADD
    case 0x5:
        result = a + op2 + carry;
        break; // ADC
    case 0x6:
        result = a - op2 + carry - 1;
        break; // SBC
    case 0x7:
        result = op2 - a + carry - 1;
        break; // RSC
    case 0x8:
        result = a & op2;
        break; // TST (no writeback)
    case 0x9:
        result = a ^ op2;
        break; // TEQ (no writeback)
    case 0xA:
        result = a - op2;
        break; // CMP(no writeback)
    case 0xB:
        result = a + op2;
        break; // CMN (no writeback)
    case 0xC:
        result = a | op2;
        break; // ORR
    case 0xD:
        result = op2;
        break; // MOV
    case 0xE:
        result = a & ~op2;
        break; // BIC
    case 0xF:
        result = ~op2;
        break; // MVN
    }

    // Update flags (if S bit set)
    if (set_cc)
    {
        bool n = (result >> 31) & 1;
        bool z = (result & 0xFFFFFFFF) == 0;
        bool c = result > 0xFFFFFFFF;
        bool v = false;

        // Overflow only meaningful for arithmetic ops
        if (op == 0x2 || op == 0xA) // SUB, CMP
            v = ((a ^ op2) & (a ^ (uint32_t)result)) >> 31;
        else if (op == 0x4 || op == 0x8) // ADD, CMN
            v = (~(a ^ op2) & (a ^ (uint32_t)result)) >> 31;

        cpsr = (cpsr & 0x0FFFFFFF) | (n << 31) | (z << 30) | (c << 29) | (v << 28);
    }

    // Writeback (TST/TEQ/CMP/CMN don't write result)
    if (op < 0x8 || op > 0xB)
        r[rd] = (uint32_t)result;

    return 1;
}

int CPU::op_ldr_str(uint32_t opcode, Memory &mem)
{
    bool imm = !((opcode >> 25) & 1);  // confusing inverted in LDR/STR
    bool pre = (opcode >> 24) & 1;     // pre vs post index
    bool up = (opcode >> 23) & 1;      // add or substract offset
    bool byte = (opcode >> 22) & 1;    // byte or word transfer
    bool load = (opcode >> 20) & 1;    // load or store
    uint8_t rn = (opcode >> 16) & 0xF; // base register
    uint8_t rd = (opcode >> 12) & 0xF; // source/dest register

    uint32_t offset = imm
                          ? opcode & 0xFFF          // 12-bit immediate
                          : read_reg(opcode & 0xF); // register offset (simplified)

    uint32_t addr = read_reg(rn);
    if (pre)
        addr = up ? addr + offset : addr - offset;

    if (load)
    {
        r[rd] = byte ? mem.read8(addr) : mem.read32(addr);
    }
    else
    {
        if (byte)
            mem.write8(addr, r[rd] & 0xFF);
        else
            mem.write32(addr, r[rd]);
    }

    if (!pre)
        addr = up ? r[rn] + offset : r[rn] - offset;

    // Writeback
    if (!pre || ((opcode >> 21) & 1))
        r[rn] = addr;

    return load ? 3 : 2;
}

int CPU::execute_thumb(uint16_t opcode, Memory &mem)
{
    uint8_t top5 = (opcode >> 11) & 0x1F;
    uint8_t top6 = (opcode >> 10) & 0x3F;

    switch (top5)
    {
    case 0b00000: // Format 1: LSL
    case 0b00001: // Format 1: LSR
    case 0b00010: // Format 1: ASR
        return thumb_format1(opcode, mem);

    case 0b00011: // FOrmat 2: ADD/SUB
        return thumb_format2(opcode, mem);

    case 0b00100:
    case 0b00101:
    case 0b00110:
    case 0b00111: // Format 3
        return thumb_format3(opcode, mem);

    case 0b01000: // Could be Format 4 (ALU) or Format 5 (hi-reg/BX)
        if (top6 == 0b010000)
            return thumb_format4(opcode, mem);
        else // top6 == 0b010001
            return thumb_format5(opcode, mem);

    default:
        // Not implemented yet - log once per unique opcode pettern so we can
        // see what's actually needed next, without flooding the console
        static bool seen[32] = {false};
        if (!seen[top5])
        {
            seen[top5] = true;
            printf("[CPU] Unimplemented Thumb format, top5=0x%02X opcode=0x%04X pc=0x%08X\n", top5, opcode, pc());
        }
        return 1;
    }
}

int CPU::thumb_format1(uint16_t opcode, Memory &mem)
{
    // Format 1: 000 Op Offset5 Rs Rd (LSL/LSR by immediate)
    uint8_t op = (opcode >> 11) & 0x3;
    uint8_t offset5 = (opcode >> 6) & 0x1F;
    uint8_t rs = (opcode >> 3) & 0x7;
    uint8_t rd = opcode & 0x7;

    uint32_t src = r[rs];
    uint32_t result = 0;
    bool carry = flag_c();

    switch (op)
    {
    case 0: // LSL
        if (offset5 == 0)
        {
            result = src;
        }
        else
        {
            result = src << offset5;
            carry = (src >> (32 - offset5)) & 1;
        }
        break;

    case 1: // LSR
        if (offset5 == 0)
        {
            result = 0;
            carry = (src >> 31) & 1;
        } // LSR #32
        else
        {
            result = src >> offset5;
            carry = (src >> (offset5 - 1)) & 1;
        }
        break;

    case 2: // ASR
        if (offset5 == 0)
        {
            result = (int32_t)src >> 31;
            carry = (src >> 31) & 1;
        } // ASR #32
        else
        {
            result = (int32_t)src >> offset5;
            carry = (src >> (offset5 - 1)) & 1;
        }
        break;
    }

    r[rd] = result;

    bool n = (result >> 31) & 1;
    bool z = result == 0;
    cpsr = (cpsr & 0x1FFFFFFF) | (n << 31) | (z << 30) | (carry << 29);

    return 1;
}

int CPU::thumb_format2(uint16_t opcode, Memory &mem)
{
    // Format 2: 00011 Op I Rn/Offset3 Rs Rd (ADD/SUB register or immediate)
    bool imm = (opcode >> 10) & 1;
    bool sub = (opcode >> 9) & 1;
    uint8_t rn_or_imm = (opcode >> 6) & 0x7;
    uint8_t rs = (opcode >> 3) & 0x7;
    uint8_t rd = opcode & 0x7;

    uint32_t a = r[rs];
    uint32_t b = imm ? rn_or_imm : r[rn_or_imm];
    uint64_t result = sub ? (a - b) : (a + b);

    bool n = (result >> 31) & 1;
    bool z = (result & 0xFFFFFFFF) == 0;
    bool c, v;

    if (sub)
    {
        c = a >= b;
        v = ((a ^ b) & (a ^ (uint32_t)result)) >> 31;
    }
    else
    {
        c = result > 0xFFFFFFFF;
        v = (~(a ^ b) & (a ^ (uint32_t)result)) >> 31;
    }

    cpsr = (cpsr & 0x0FFFFFFF) | (n << 31) | (z << 30) | (c << 29) | (v << 28);
    r[rd] = (uint32_t)result;

    return 1;
}

int CPU::thumb_format3(uint16_t opcode, Memory &mem)
{
    // Format 3: 001 op Rd Offset8
    // Bits: [15-13]=001 [12-11]=op [10-8]=Rd [7-0]=Offset8
    uint8_t op = (opcode >> 11) & 0x3; // 0=MOV 1=CMP 2=ADD 3=SUB
    uint8_t rd = (opcode >> 8) & 0x7;  // only r0-r7 reachable in Thumb low regs
    uint8_t offset8 = opcode & 0xFF;

    uint32_t a = r[rd];
    uint64_t result = 0;
    bool do_writeback = true;

    switch (op)
    {
    case 0: // MOV Rd, #Offset8
        result = offset8;
        break;
    case 1: // CMP Rd, #Offset8
        result = a - offset8;
        do_writeback = false;
        break;
    case 2: // ADD Rd, #Offset8
        result = a + offset8;
        break;
    case 3: // SUB Rd, #Offset8
        result = a - offset8;
        break;
    }

    // Format 3 always sets flags (no S bit choice in Thumb for these)
    bool n = (result >> 31) & 1;
    bool z = (result & 0xFFFFFFFF) == 0;
    bool c, v;

    if (op == 0) // MOV doesn't affect C/V
    {
        c = flag_c();
        v = flag_v();
    }
    else if (op == 2) // ADD
    {
        c = result > 0xFFFFFFFF;
        v = (~(a ^ offset8) & (a ^ (uint32_t)result)) >> 31;
    }
    else // CMP and SUB (both substraction)
    {
        c = a >= offset8;
        v = ((a ^ offset8) & (a ^ (uint32_t)result)) >> 31;
    }

    cpsr = (cpsr & 0x0FFFFFFF) | (n << 31) | (z << 30) | (c << 29) | (v << 28);

    if (do_writeback)
        r[rd] = (uint32_t)result;

    return 1;
}

int CPU::thumb_format4(uint16_t opcode, Memory &mem)
{
    // Format 4: 010000 Op Rs Rd (ALU operations, low registers only)
    uint8_t op = (opcode >> 6) & 0xF;
    uint8_t rs = (opcode >> 3) & 0x7;
    uint8_t rd = opcode & 0x7;

    uint32_t a = r[rd];
    uint32_t b = r[rs];
    uint64_t result = 0;
    bool set_flags = true;
    bool write = true;
    bool carry = flag_c();
    bool overflow = flag_v();
    bool is_arith = false; // true for ADC/SBC/CMP/NEG - needs c/v recompute

    switch (op)
    {
    case 0x0:
        result = a & b;
        break; // AND
    case 0x1:
        result = a ^ b;
        break; // EOR
    case 0x2:
        result = a << (b & 0xFF);
        if ((b & 0xFF))
            carry = (a >> (32 - (b & 0xFF))) & 1;
        break; // LSL
    case 0x3:
        result = (b & 0xFF) == 0 ? a : a >> (b & 0xFF);
        if ((b & 0xFF))
            carry = (a >> ((b & 0xFF) - 1)) & 1;
        break; // LSR
    case 0x4:
        result = (b & 0x1F) == 0 ? a : (uint32_t)((int32_t)a >> (b & 0xFF));
        if ((b & 0xFF))
            carry = (a >> ((b & 0xFF) - 1)) & 1;
        break; // ASR
    case 0x5:
        result = (uint64_t)a + b + carry;
        is_arith = true;
        break; // ADC
    case 0x6:
        result = (uint64_t)a - b - !carry;
        is_arith = true;
        break; // SBC
    case 0x7:
        result = (b & 0x1F) == 0 ? a : (a >> (b & 0x1F)) | (a << (32 - (b & 0x1F)));
        carry = (result >> 31) & 1;
        break; // ROR
    case 0x8:
        result = a & b;
        write = false;
        break; // TST
    case 0x9:
        result = 0 - b;
        is_arith = true;
        a = 0;
        write = true;
        rd = rd;
        break; // NEG (handled below properly)
    case 0xA:
        result = (uint64_t)a - b;
        is_arith = true;
        write = false;
        break; // CMP
    case 0xB:
        result = (uint64_t)a + b;
        is_arith = true;
        write = false;
        break; // CMN
    case 0xC:
        result = a | b;
        break;
    case 0xD:
        result = (uint64_t)a * b;
        break;
    case 0xE:
        result = a & ~b;
        break;
    case 0xF:
        result = ~b;
        break;
    }

    // Fix NEG properly: NEG Rd, Rs = 0 - Rs
    if (op == 0x9)
    {
        uint32_t rs_val = r[rs];
        result = (uint64_t)0 - rs_val;
        bool c2 = 0u >= rs_val;
        bool v2 = ((0u ^ rs_val) & (0u ^ (uint32_t)result)) >> 31;
        carry = c2;
        overflow = v2;
    }
    else if (is_arith)
    {
        uint32_t opa = (op == 0x9) ? 0 : a;
        uint32_t opb = b;
        bool sub_op = (op == 0x6 || op == 0xA || op == 0x9); // SBC, CMP, NEG behave as substraction
        if (sub_op)
        {
            carry = opa >= opb;
            overflow = ((opa ^ opb) & (opa ^ (uint32_t)result)) >> 31;
        }
        else // ADC, CMN
        {
            carry = result > 0xFFFFFFFF;
            overflow = (~(opa ^ opb) & (opa ^ (uint32_t)result)) >> 31;
        }
    }

    bool n = (result >> 31) & 1;
    bool z = (result & 0xFFFFFFFF) == 0;

    if (set_flags)
        cpsr = (cpsr & 0x0FFFFFFF) | (n << 31) | (z << 30) | (carry << 29) | (overflow << 28);

    if (write)
        r[rd] = (uint32_t)result;

    return 1;
}

int CPU::thumb_format5(uint16_t opcode, Memory &mem)
{
    // Format 5: 010001 Op H1 H2 Rs/Hs Rd/Hd (Hi register ops / BX) also hi
    uint8_t op = (opcode >> 8) & 0x3;
    bool h1 = (opcode >> 7) & 1;
    bool h2 = (opcode >> 6) & 1;
    uint8_t rs = ((opcode >> 3) & 0x7) | (h2 ? 8 : 0);
    uint8_t rd = (opcode & 0x7) | (h1 ? 8 : 0);

    uint32_t src = read_reg(rs);

    switch (op)
    {
    case 0: // ADD
        r[rd] = r[rd] + src;
        if (rd == 15)
            r[15] &= ~1u;
        break;
    case 1: // CMP
    {
        uint32_t a = read_reg(rd);
        uint64_t result = (uint64_t)a - src;
        bool n = (result >> 31) & 1;
        bool z = (result & 0xFFFFFFFF) == 0;
        bool c = a >= src;
        bool v = ((a ^ src) & (a ^ (uint32_t)result)) >> 31;
        cpsr = (cpsr & 0x0FFFFFFF) | (n << 31) | (z << 30) | (c << 29) | (v << 28);
        break;
    }

    case 2: // MOV
        r[rd] = src;
        if (rd == 15)
            r[15] &= ~1u;
        break;

    case 3: // BX
        if (src & 1)
        {
            cpsr |= (1 << 5);
            pc() = src & ~1u;
        }
        else
        {
            cpsr &= ~(1 << 5);
            pc() = src & ~3u;
        }
        break;
    }

    return 1;
}