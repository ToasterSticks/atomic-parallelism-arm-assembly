#ifndef _MEM_H_
#define _MEM_H_

#include <cstdint>

uint8_t mem_read8(uint64_t address);
uint32_t mem_read32(uint64_t address);
uint64_t mem_read64(uint64_t address);
void mem_write8(uint64_t address, uint8_t data);
void mem_write32(uint64_t address, uint32_t data);
void mem_write64(uint64_t address, uint64_t data);
uint64_t mem_ldaddal(uint64_t address, uint64_t addend);
uint64_t mem_casal(uint64_t address, uint64_t expected, uint64_t new_val);
uint32_t mem_ldaddal32(uint64_t address, uint32_t addend);
uint32_t mem_casal32(uint64_t address, uint32_t expected, uint32_t new_val);
void mem_lock_output();
void mem_unlock_output();

#endif
