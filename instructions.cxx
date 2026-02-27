#include "instructions.h"
#include "mem.h"

// Consulted AI for bit extraction helper of a contiguous range
static uint64_t extractBits(uint64_t value, uint8_t lsb, uint8_t width) {
    if (width == 0 || width > 64 || lsb >= 64) {
        return 0;
    }
    if ((uint32_t)(lsb) + (uint32_t)(width) > 64) {
        return 0;
    }
    if (width == 64) {
        return value;
    }
    uint64_t mask = (1ULL << width) - 1ULL;
    return (value >> lsb) & mask;
}

static Word bitIndex(Word value, uint8_t index) {
    if (index >= 32) {
        return 0;
    }
    return (value >> index) & 0x1;
}

static int64_t signExtendTo64(uint64_t value, uint8_t width) {
    if (width == 0 || width >= 64) {
        return (int64_t)(value);
    }
    uint8_t shift = (uint8_t)(64 - width);
    return (int64_t)(value << shift) >> shift;
}

static uint64_t addOffset(uint64_t base, int64_t offset) {
    return base + (uint64_t)(offset);
}

static bool conditionHolds(ThreadState const& state, Word cond) {
    bool n = state.n == 1;
    bool z = state.z == 1;
    bool c = state.c == 1;
    bool v = state.v == 1;
    switch (cond & 0xF) {
        case 0x0:
            return z;
        case 0x1:
            return !z;
        case 0x2:
            return c;
        case 0x3:
            return !c;
        case 0x4:
            return n;
        case 0x5:
            return !n;
        case 0x6:
            return v;
        case 0x7:
            return !v;
        case 0x8:
            return c && !z;
        case 0x9:
            return !c || z;
        case 0xA:
            return n == v;
        case 0xB:
            return n != v;
        case 0xC:
            return !z && (n == v);
        case 0xD:
            return z || (n != v);
        case 0xE:
            return true;
        default:
            return false;
    }
}

void executeSubsShifted(ThreadState& state, Word word) {
    Word sf = extractBits(word, 31, 1);
    Word shift_type = extractBits(word, 22, 2);
    Word rm = extractBits(word, 16, 5);
    Word imm6 = extractBits(word, 10, 6);
    Word rn = extractBits(word, 5, 5);
    Word rd = extractBits(word, 0, 5);
    bool is64 = sf == 1;
    uint64_t mask = is64 ? ~0ULL : 0xFFFFFFFF;
    uint64_t operand1 = (rn == 31) ? 0 : state.x[rn];
    uint64_t operand2 = (rm == 31) ? 0 : state.x[rm];
    operand1 &= mask;
    operand2 &= mask;
    uint8_t shift = (uint8_t)(imm6 & (is64 ? 63 : 31));
    uint64_t shifted = operand2;
    if (shift_type == 0) {
        shifted = (operand2 << shift) & mask;
    } else if (shift_type == 1) {
        shifted = (operand2 >> shift) & mask;
    } else if (shift_type == 2) {
        if (is64) {
            shifted = (uint64_t)((int64_t)(operand2) >> shift) & mask;
        } else {
            shifted = (uint32_t)((int32_t)(operand2) >> shift);
        }
    } else {
        shifted = operand2;
    }
    uint64_t result = (operand1 - shifted) & mask;
    if (rd != 31) {
        state.x[rd] = is64 ? result : (uint32_t)(result);
    }
    uint64_t sign = is64 ? (1ULL << 63) : (1ULL << 31);
    state.n = ((result & sign) != 0) ? 1 : 0;
    state.z = (result == 0) ? 1 : 0;
    state.c = (operand1 >= shifted) ? 1 : 0;
    state.v = (((operand1 ^ shifted) & (operand1 ^ result) & sign) != 0) ? 1 : 0;
}

void executeBCond(ThreadState& state, Word word, bool& pc_overridden) {
    Word imm19 = extractBits(word, 5, 19);
    Word cond = extractBits(word, 0, 4);
    if (!conditionHolds(state, cond)) {
        return;
    }
    int64_t offset = signExtendTo64((uint64_t)(imm19) << 2, 21);
    state.pc = addOffset(state.pc, offset);
    pc_overridden = true;
}

void executeCsel(ThreadState& state, Word word) {
    
}

void executeAdrp(ThreadState& state, Word word) {
    Word rd = extractBits(word, 0, 5);
    Word immlo = extractBits(word, 29, 2);
    Word immhi = extractBits(word, 5, 19);
    uint64_t imm21 = ((uint64_t)(immhi) << 2) | immlo;
    int64_t signed_imm21 = signExtendTo64(imm21, 21);
    int64_t offset = signed_imm21 << 12;
    uint64_t base = extractBits(state.pc, 12, 52) << 12;
    uint64_t result = addOffset(base, offset);
    if (rd == 31) {
        return;
    }
    state.x[rd] = result;
}

void executeAddi(ThreadState& state, Word word) {
    Word sf = extractBits(word, 31, 1);
    Word sh = extractBits(word, 22, 1);
    Word imm12 = extractBits(word, 10, 12);
    Word rn = extractBits(word, 5, 5);
    Word rd = extractBits(word, 0, 5);
    bool is32 = (sf == 0);
    uint64_t imm = imm12;
    if (sh == 1) {
        imm <<= 12;
    }
    uint64_t operand1;
    if (rn == 31) {
        operand1 = is32 ? (uint32_t)(state.sp) : state.sp;
    } else {
        uint64_t xn = state.x[rn];
        operand1 = is32 ? (uint32_t)(xn) : xn;
    }
    uint64_t result =
        is32 ? (uint32_t)(operand1 + imm) : (operand1 + imm);
    if (rd == 31) {
        state.sp = is32 ? (uint32_t)(result) : result;
    } else {
        state.x[rd] = is32 ? (uint32_t)(result) : result;
    }
}

void executeLdrImm(ThreadState& state, Word word) {
    Word size = extractBits(word, 30, 2);
    Word rn = extractBits(word, 5, 5);
    Word rt = extractBits(word, 0, 5);
    bool is64 = size == 3;
    bool unsigned_offset = extractBits(word, 24, 1) == 1;
    bool wback = !unsigned_offset;
    bool postindex = false;
    int64_t offset = 0;
    if (unsigned_offset) {
        Word imm12 = extractBits(word, 10, 12);
        offset = (int64_t)(imm12 << (is64 ? 3 : 2));
    } else {
        Word imm9 = extractBits(word, 12, 9);
        offset = signExtendTo64(imm9, 9);
        postindex = extractBits(word, 11, 1) == 0;
    }
    uint64_t address = (rn == 31) ? state.sp : state.x[rn];
    if (!postindex) {
        address = addOffset(address, offset);
    }
    uint64_t loaded = is64 ? mem_read64(address) : mem_read32(address);
    if (rt != 31) {
        state.x[rt] = is64 ? loaded : (uint32_t)(loaded);
    }
    if (wback) {
        if (postindex) {
            address = addOffset(address, offset);
        }
        if (rn == 31) {
            state.sp = address;
        } else {
            state.x[rn] = address;
        }
    }
}

void executeMovz(ThreadState& state, Word word) {
    Word sf = extractBits(word, 31, 1);
    Word hw = extractBits(word, 21, 2);
    Word imm16 = extractBits(word, 5, 16);
    Word rd = extractBits(word, 0, 5);
    if (sf == 0 && bitIndex(hw, 1) == 1) {
        return;
    }
    if (rd == 31) {
        return;
    }
    uint64_t value = (uint64_t)(imm16) << ((uint64_t)(hw) * 16);
    if (sf == 0) {
        state.x[rd] = (uint32_t)(value);
    } else {
        state.x[rd] = value;
    }
}

void executeOrrImm(ThreadState& state, Word word) {
    
}

void executeStrImm(ThreadState& state, Word word) {
    
}

void executeStrbImm(ThreadState& state, Word word) {
    Word rt = extractBits(word, 0, 5);
    Word rn = extractBits(word, 5, 5);
    bool unsigned_offset = extractBits(word, 24, 1) == 1;
    bool wback = !unsigned_offset;
    bool postindex = false;
    int64_t offset = 0;

    if (unsigned_offset) {
        Word imm12 = extractBits(word, 10, 12);
        offset = imm12;
    } else {
        Word imm9 = extractBits(word, 12, 9);
        offset = signExtendTo64(imm9, 9);
        postindex = extractBits(word, 11, 1) == 0;
    }

    uint64_t address = (rn == 31) ? state.sp : state.x[rn];
    if (!postindex) {
        address = addOffset(address, offset);
    }

    uint8_t data = (rt == 31) ? 0 : (uint8_t)(state.x[rt]);
    mem_write8(address, data);

    if (wback) {
        if (postindex) {
            address = addOffset(address, offset);
        }
        if (rn == 31) {
            state.sp = address;
        } else {
            state.x[rn] = address;
        }
    }
}

void executeLdaddal(ThreadState& state, Word word) {
    Word size = extractBits(word, 30, 2);
    Word rs = extractBits(word, 16, 5);
    Word rn = extractBits(word, 5, 5);
    Word rt = extractBits(word, 0, 5);
    bool is64 = size == 3;
    uint64_t address = (rn == 31) ? state.sp : state.x[rn];
    if (is64) {
        uint64_t addend = (rs == 31) ? 0 : state.x[rs];
        uint64_t old = mem_ldaddal(address, addend);
        if (rt != 31) {
            state.x[rt] = old;
        }
        return;
    }
    uint32_t addend32 = (rs == 31) ? 0 : (uint32_t)(state.x[rs]);
    uint32_t old32 = mem_ldaddal32(address, addend32);
    if (rt != 31) {
        state.x[rt] = old32;
    }
}

void executeCasal(ThreadState& state, Word word) {
    Word size = extractBits(word, 30, 2);
    Word rs = extractBits(word, 16, 5);
    Word rn = extractBits(word, 5, 5);
    Word rt = extractBits(word, 0, 5);
    bool is64 = size == 3;
    uint64_t address = (rn == 31) ? state.sp : state.x[rn];
    if (is64) {
        uint64_t expected = (rs == 31) ? 0 : state.x[rs];
        uint64_t new_val = (rt == 31) ? 0 : state.x[rt];
        uint64_t old = mem_casal(address, expected, new_val);
        if (rs != 31) {
            state.x[rs] = old;
        }
        return;
    }
    uint32_t expected32 = (rs == 31) ? 0 : (uint32_t)(state.x[rs]);
    uint32_t new_val32 = (rt == 31) ? 0 : (uint32_t)(state.x[rt]);
    uint32_t old32 = mem_casal32(address, expected32, new_val32);
    if (rs != 31) {
        state.x[rs] = old32;
    }
}
