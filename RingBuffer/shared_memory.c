#include "shared_memory.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

RingBuffer* create_anonymous_shared_memory(size_t size) {
    void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    return (RingBuffer*)addr;
}
