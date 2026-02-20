#ifndef _MEM_H_
#define _MEM_H_

#include <cstdint>

// This is the memory interface
// We assume that memory is zero-filled


// Write the given byte value at the given address
extern void mem_write8(uint64_t address, uint8_t data);

#endif
