#include <thread>
#include <stdio.h>
#include <stdlib.h>

#include "elf.h"
#include "arm.h"

constexpr static int N = 4;

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s <ARM executable file name>\n",argv[0]);
        exit(-1);
    }

    std::thread threads[N];

    uint64_t entry = loadElf(argv[1]);
    for (int i=0; i<N; i++) {
        threads[i] = std::thread([entry, i] {
            emulate(entry, i);
        });
    }

    // Wait for all threads to finish.
    for (int i=0; i<N; i++) {
        threads[i].join();
    }

    destroy(); // For any cleanup if needed.

    return 0;
}
