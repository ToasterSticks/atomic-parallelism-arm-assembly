#pragma once

#include <stdint.h>

struct ThreadState {
    uint64_t x[31];
    uint64_t sp;
    uint64_t pc;
    int thread_id;
    uint8_t n, z, c, v;
};

extern void emulate(uint64_t entry, int thread_id);
