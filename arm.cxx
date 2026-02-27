#include "arm.h"
#include "instructions.h"
#include "mem.h"
#include "debug.h"
#include <cstdint>
#include <cstdio>

constexpr uint64_t instruction_bytes = 4;
using PrintHex64 = unsigned long long;

struct DecodePattern {
    Word mask;
    Word match;
    Opcode opcode;
};

// Used AI to generate bit masks and matches from README
static constexpr DecodePattern decode_patterns[] = {
    {0x7F200000, 0x6B000000, Opcode::subs_shifted},
    {0xFF000010, 0x54000000, Opcode::b_cond},
    {0x7FE00C00, 0x1A800000, Opcode::csel},
    {0x9F000000, 0x90000000, Opcode::adrp},
    {0x7F800000, 0x11000000, Opcode::addi},
    {0xBFE00400, 0xB8400400, Opcode::ldr},
    {0xBFC00000, 0xB9400000, Opcode::ldr},
    {0x7F800000, 0x52800000, Opcode::movz},
    {0x7F800000, 0x32000000, Opcode::orr},
    {0xBFE00400, 0xB8000400, Opcode::str},
    {0xBFC00000, 0xB9000000, Opcode::str},
    {0xFFE00400, 0x38000400, Opcode::strb},
    {0xFFC00000, 0x39000000, Opcode::strb},
    {0xBFE0FC00, 0xB8E00000, Opcode::ldaddal},
    {0xBFE0FC00, 0x88E0FC00, Opcode::casal},
};

bool matchesPattern(Word word, DecodePattern const& pattern) {
    return (word & pattern.mask) == pattern.match;
}

Opcode decodeOpcode(Word word) {
    for (DecodePattern const& pattern : decode_patterns) {
        if (matchesPattern(word, pattern)) {
            return pattern.opcode;
        }
    }
    return Opcode::unknown;
}

ThreadState makeThreadState(uint64_t entry, int thread_id) {
    ThreadState state {};
    state.pc = entry;
    state.thread_id = thread_id;
    state.x[0] = thread_id;
    return state;
}

void advancePc(ThreadState& state) {
    state.pc += instruction_bytes;
}

void printUnknownInstruction(Word word, ThreadState const& state) {
    if (state.thread_id != 0) return;
    mem_lock_output();
    printf("unknown instruction %08x at %llx\n", word, (PrintHex64) state.pc);
    for (int i = 0; i < 31; i++) {
        printf("X%02d : %016llX\n", i, (PrintHex64) state.x[i]);
    }
    printf("XSP : %016llX\n", (PrintHex64) state.sp);
    mem_unlock_output();
}

bool executeInstructionWord(ThreadState& state, Word word, bool& pc_overridden) {
    Opcode opcode = decodeOpcode(word);
    pc_overridden = false;
    switch (opcode) {
        case Opcode::subs_shifted:
            executeSubsShifted(state, word);
            return true;
        case Opcode::b_cond:
            executeBCond(state, word, pc_overridden);
            return true;
        case Opcode::csel:
            executeCsel(state, word);
            return true;
        case Opcode::adrp:
            executeAdrp(state, word);
            return true;
        case Opcode::addi:
            executeAddi(state, word);
            return true;
        case Opcode::ldr:
            executeLdrImm(state, word);
            return true;
        case Opcode::movz:
            executeMovz(state, word);
            return true;
        case Opcode::orr:
            executeOrrImm(state, word);
            return true;
        case Opcode::str:
            executeStrImm(state, word);
            return true;
        case Opcode::strb:
            executeStrbImm(state, word);
            return true;
        case Opcode::ldaddal:
            executeLdaddal(state, word);
            return true;
        case Opcode::casal:
            executeCasal(state, word);
            return true;
        case Opcode::unknown:
            return false;
    }
    return false;
}

void emulate(uint64_t entry, int thread_id) {
    ThreadState state = makeThreadState(entry, thread_id);
    while (true) {
        Word instruction = mem_read32(state.pc);
        bool pc_overridden = false;
        bool known = executeInstructionWord(state, instruction, pc_overridden);
        if (!known) {
            return printUnknownInstruction(instruction, state);
        }
        if (!pc_overridden) {
            advancePc(state);
        }
    }
}
