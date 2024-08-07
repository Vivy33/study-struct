#include "ringbuffer.h"
#include <string.h>
#include <stdatomic.h>

void ringbuffer_init(RingBuffer* rb) {
    atomic_store(&rb->head, 0);
    atomic_store(&rb->tail, 0);
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

    int initial_head = atomic_load(&rb->head);
    int initial_tail = atomic_load(&rb->tail);
    int available_data = (initial_head - initial_tail + rb->size) % rb->size;

    int count = length < available_data ? length : available_data;

    for (int i = 0; i < count; i++) {
        data[i] = rb->buffer[atomic_load(&rb->tail)];
        atomic_store(&rb->tail, (atomic_load(&rb->tail) + 1) % rb->size);
    }

    return count;
}
