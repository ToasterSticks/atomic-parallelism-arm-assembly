#include "mem.h"
#include <cstdio>
#include <mutex>
#include <unordered_map>

using namespace std;

static unordered_map<uint64_t, uint8_t> memory_bytes;
static mutex memory_mutex;

static uint8_t read8Unlocked(uint64_t address) {
    auto it = memory_bytes.find(address);
    if (it == memory_bytes.end()) {
        return 0;
    }
    return it->second;
}

static void write8Unlocked(uint64_t address, uint8_t data) {
    if (data == 0) {
        memory_bytes.erase(address);
        return;
    }
    memory_bytes[address] = data;
}

static uint64_t read64Unlocked(uint64_t address) {
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= (uint64_t) read8Unlocked(address + (uint64_t) i) << (8 * i);
    }
    return value;
}

static uint32_t read32Unlocked(uint64_t address) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value |= (uint32_t) read8Unlocked(address + (uint64_t) i) << (8 * i);
    }
    return value;
}

static void write64Unlocked(uint64_t address, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        uint8_t byte = (uint8_t) ((value >> (8 * i)) & 0xff);
        write8Unlocked(address + (uint64_t) i, byte);
    }
}

static void write32Unlocked(uint64_t address, uint32_t value) {
    for (int i = 0; i < 4; i++) {
        uint8_t byte = (uint8_t) ((value >> (8 * i)) & 0xff);
        write8Unlocked(address + (uint64_t) i, byte);
    }
}

uint8_t mem_read8(uint64_t address) {
    scoped_lock lock(memory_mutex);
    return read8Unlocked(address);
}

uint32_t mem_read32(uint64_t address) {
    scoped_lock lock(memory_mutex);
    return read32Unlocked(address);
}

uint64_t mem_read64(uint64_t address) {
    scoped_lock lock(memory_mutex);
    return read64Unlocked(address);
}

void mem_write8(uint64_t address, uint8_t data) {
    scoped_lock lock(memory_mutex);
    if (address == UINT64_MAX) {
        putchar((int) data);
        return;
    }
    write8Unlocked(address, data);
}

void mem_write32(uint64_t address, uint32_t data) {
    scoped_lock lock(memory_mutex);
    write32Unlocked(address, data);
}

void mem_write64(uint64_t address, uint64_t data) {
    scoped_lock lock(memory_mutex);
    write64Unlocked(address, data);
}

uint64_t mem_ldaddal(uint64_t address, uint64_t addend) {
    scoped_lock lock(memory_mutex);
    uint64_t old = read64Unlocked(address);
    write64Unlocked(address, old + addend);
    return old;
}

uint64_t mem_casal(uint64_t address, uint64_t expected, uint64_t new_val) {
    scoped_lock lock(memory_mutex);
    uint64_t old = read64Unlocked(address);
    if (old == expected) {
        write64Unlocked(address, new_val);
    }
    return old;
}

uint32_t mem_ldaddal32(uint64_t address, uint32_t addend) {
    scoped_lock lock(memory_mutex);
    uint32_t old = read32Unlocked(address);
    write32Unlocked(address, old + addend);
    return old;
}

uint32_t mem_casal32(uint64_t address, uint32_t expected, uint32_t new_val) {
    scoped_lock lock(memory_mutex);
    uint32_t old = read32Unlocked(address);
    if (old == expected) {
        write32Unlocked(address, new_val);
    }
    return old;
}

void initialize() {}
void destroy() {
    scoped_lock lock(memory_mutex);
    memory_bytes.clear();
}
