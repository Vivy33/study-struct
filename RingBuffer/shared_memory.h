#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "ringbuffer.h"
#include <stddef.h>

int create_shared_memory(const char* name, size_t size);
RingBuffer* map_shared_memory(int fd, size_t size);

#endif // SHARED_MEMORY_H
