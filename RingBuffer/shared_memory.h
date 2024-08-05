#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "ringbuffer.h"
#include <stddef.h>

RingBuffer* create_anonymous_shared_memory(size_t size);

#endif // SHARED_MEMORY_H
