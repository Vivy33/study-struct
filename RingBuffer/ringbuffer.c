#include "ringbuffer.h"
#include <string.h>
#include <stdatomic.h>

void ringbuffer_init(RingBuffer* rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->size = RINGBUFFER_SIZE;
}

bool ringbuffer_is_empty(const RingBuffer* rb) {
    return atomic_load(&rb->head) == atomic_load(&rb->tail);
}

bool ringbuffer_is_full(const RingBuffer* rb) {
    return ((atomic_load(&rb->head) + 1) % rb->size) == atomic_load(&rb->tail);
}

bool ringbuffer_write(RingBuffer* rb, const char* data, int length) {
    if (length > rb->size - 1 || ringbuffer_is_full(rb)) {
        return false;
    }

    for (int i = 0; i < length; i++) {
        rb->buffer[atomic_load(&rb->head)] = data[i];
        atomic_store(&rb->head, (atomic_load(&rb->head) + 1) % rb->size);
    }

    return true;
}

int ringbuffer_read(RingBuffer* rb, char* data, int length) {
    if (ringbuffer_is_empty(rb)) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < length; i++) {
        if (ringbuffer_is_empty(rb)) {
            break;
        }
        data[i] = rb->buffer[atomic_load(&rb->tail)];
        atomic_store(&rb->tail, (atomic_load(&rb->tail) + 1) % rb->size);
        count++;
    }

    return count;
}
