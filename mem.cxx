#include "mem.h"
#include <cstdlib>
#include <cstdio>
#include <mutex>
#include <unordered_map>

using namespace std;

constexpr uint64_t lower_memory_size = 1ULL << 32;
constexpr int mem_order = __ATOMIC_SEQ_CST;

static uint8_t* lower_memory_bytes = nullptr;
static unordered_map<uint64_t, uint8_t> memory_bytes;
static mutex memory_mutex;

static bool in_bounds(uint64_t address, uint64_t size) {
    if (size == 0 || address >= lower_memory_size) {
        return false;
    }
    return address <= (lower_memory_size - size);
}

static uint8_t read8_unlocked(uint64_t address) {
    if (in_bounds(address, 1) && lower_memory_bytes) {
        return __atomic_load_n(lower_memory_bytes + address, mem_order);
    }
    auto it = memory_bytes.find(address);
    if (it == memory_bytes.end()) {
        return 0;
    }
    return it->second;
}

static void write8_unlocked(uint64_t address, uint8_t data) {
    if (address == UINT64_MAX) {
        putchar((int) data);
        return;
    }
    if (in_bounds(address, 1) && lower_memory_bytes) {
        __atomic_store_n(lower_memory_bytes + address, data, mem_order);
        return;
    }
    if (data == 0) {
        memory_bytes.erase(address);
        return;
    }
    memory_bytes[address] = data;
}

static uint32_t read32_unlocked(uint64_t address) {
    if ((address & 0x3ULL) == 0 && in_bounds(address, 4) && lower_memory_bytes) {
        return __atomic_load_n((uint32_t*) (lower_memory_bytes + address), mem_order);
    }
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value |= (uint32_t) read8_unlocked(address + (uint64_t) i) << (8 * i);
    }
    return value;
}

static uint64_t read64_unlocked(uint64_t address) {
    if ((address & 0x7ULL) == 0 && in_bounds(address, 8) && lower_memory_bytes) {
        return __atomic_load_n((uint64_t*) (lower_memory_bytes + address), mem_order);
    }
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= (uint64_t) read8_unlocked(address + (uint64_t) i) << (8 * i);
    }
    return value;
}

static void write32_unlocked(uint64_t address, uint32_t value) {
    if ((address & 0x3ULL) == 0 && in_bounds(address, 4) && lower_memory_bytes) {
        __atomic_store_n((uint32_t*) (lower_memory_bytes + address), value, mem_order);
        return;
    }
    for (int i = 0; i < 4; i++) {
        uint8_t byte = (uint8_t) ((value >> (8 * i)) & 0xFF);
        write8_unlocked(address + (uint64_t) i, byte);
    }
}

static void write64_unlocked(uint64_t address, uint64_t value) {
    if ((address & 0x7ULL) == 0 && in_bounds(address, 8) && lower_memory_bytes) {
        __atomic_store_n((uint64_t*) (lower_memory_bytes + address), value, mem_order);
        return;
    }
    for (int i = 0; i < 8; i++) {
        uint8_t byte = (uint8_t) ((value >> (8 * i)) & 0xFF);
        write8_unlocked(address + (uint64_t) i, byte);
    }
}

uint8_t mem_read8(uint64_t address) {
    if (in_bounds(address, 1) && lower_memory_bytes) {
        return read8_unlocked(address);
    }
    scoped_lock lock(memory_mutex);
    return read8_unlocked(address);
}

uint32_t mem_read32(uint64_t address) {
    if (in_bounds(address, 4) && lower_memory_bytes) {
        return read32_unlocked(address);
    }
    scoped_lock lock(memory_mutex);
    return read32_unlocked(address);
}

uint64_t mem_read64(uint64_t address) {
    if (in_bounds(address, 8) && lower_memory_bytes) {
        return read64_unlocked(address);
    }
    scoped_lock lock(memory_mutex);
    return read64_unlocked(address);
}

void mem_write8(uint64_t address, uint8_t data) {
    if (in_bounds(address, 1) && lower_memory_bytes) {
        write8_unlocked(address, data);
        return;
    }
    scoped_lock lock(memory_mutex);
    write8_unlocked(address, data);
}

void mem_write32(uint64_t address, uint32_t data) {
    if (in_bounds(address, 4) && lower_memory_bytes) {
        write32_unlocked(address, data);
        return;
    }
    scoped_lock lock(memory_mutex);
    write32_unlocked(address, data);
}

void mem_write64(uint64_t address, uint64_t data) {
    if (in_bounds(address, 8) && lower_memory_bytes) {
        write64_unlocked(address, data);
        return;
    }
    scoped_lock lock(memory_mutex);
    write64_unlocked(address, data);
}

uint64_t mem_ldaddal(uint64_t address, uint64_t addend) {
    if ((address & 0x7ULL) == 0 && in_bounds(address, 8) && lower_memory_bytes) {
        return __atomic_fetch_add((uint64_t*) (lower_memory_bytes + address), addend, mem_order);
    }
    scoped_lock lock(memory_mutex);
    uint64_t old = read64_unlocked(address);
    write64_unlocked(address, old + addend);
    return old;
}

uint64_t mem_casal(uint64_t address, uint64_t expected, uint64_t new_val) {
    if ((address & 0x7ULL) == 0 && in_bounds(address, 8) && lower_memory_bytes) {
        uint64_t observed = expected;
        __atomic_compare_exchange_n((uint64_t*) (lower_memory_bytes + address), &observed, new_val, false, mem_order, mem_order);
        return observed;
    }
    scoped_lock lock(memory_mutex);
    uint64_t old = read64_unlocked(address);
    if (old == expected) {
        write64_unlocked(address, new_val);
    }
    return old;
}

uint32_t mem_ldaddal32(uint64_t address, uint32_t addend) {
    if ((address & 0x3ULL) == 0 && in_bounds(address, 4) && lower_memory_bytes) {
        return __atomic_fetch_add((uint32_t*) (lower_memory_bytes + address), addend, mem_order);
    }
    scoped_lock lock(memory_mutex);
    uint32_t old = read32_unlocked(address);
    write32_unlocked(address, old + addend);
    return old;
}

uint32_t mem_casal32(uint64_t address, uint32_t expected, uint32_t new_val) {
    if ((address & 0x3ULL) == 0 && in_bounds(address, 4) && lower_memory_bytes) {
        uint32_t observed = expected;
        __atomic_compare_exchange_n((uint32_t*) (lower_memory_bytes + address), &observed, new_val, false, mem_order, mem_order);
        return observed;
    }
    scoped_lock lock(memory_mutex);
    uint32_t old = read32_unlocked(address);
    if (old == expected) {
        write32_unlocked(address, new_val);
    }
    return old;
}

void initialize() {
    lower_memory_bytes = (uint8_t*) calloc(lower_memory_size, sizeof(uint8_t));
    if (lower_memory_bytes == nullptr) {
        return;
    }
    memory_bytes.clear();
}

void destroy() {
    if (lower_memory_bytes) {
        free(lower_memory_bytes);
        lower_memory_bytes = nullptr;
    }
    memory_bytes.clear();
}
