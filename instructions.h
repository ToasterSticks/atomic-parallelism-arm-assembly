#pragma once

#include "arm.h"
#include <cstdint>

using Word = uint32_t;

enum class Opcode {
    subs_shifted,
    b_cond,
    csel,
    adrp,
    addi,
    ldr,
    movz,
    orr,
    str,
    strb,
    ldaddal,
    casal,
    unknown
};

bool executeInstructionWord(ThreadState& state, Word word, bool& pc_overridden);
Opcode decodeOpcode(Word word);

void executeSubsShifted(ThreadState& state, Word word);
void executeBCond(ThreadState& state, Word word, bool& pc_overridden);
void executeCsel(ThreadState& state, Word word);
void executeAdrp(ThreadState& state, Word word);
void executeAddi(ThreadState& state, Word word);
void executeLdrImm(ThreadState& state, Word word);
void executeMovz(ThreadState& state, Word word);
void executeOrrImm(ThreadState& state, Word word);
void executeStrImm(ThreadState& state, Word word);
void executeStrbImm(ThreadState& state, Word word);
void executeLdaddal(ThreadState& state, Word word);
void executeCasal(ThreadState& state, Word word);
