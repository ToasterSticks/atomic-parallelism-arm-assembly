#include "elf.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

void emulate(uint64_t entry, int thread_id) {
    printf("entry = %llx. thread_id = %d.\n",(long long)entry, thread_id);
    MISSING();
}
