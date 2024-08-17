#include "ringbuffer.h"
#include <string.h>
#include <stdatomic.h>

bool ringbuffer_is_empty(const RingBuffer* rb) {
    return atomic_load(&rb->head) == atomic_load(&rb->tail);
}

bool ringbuffer_is_full(const RingBuffer* rb) {
    return ((atomic_load(&rb->head) + sizeof(DataHeader)) % rb->size) == atomic_load(&rb->tail);
}

bool ringbuffer_write(RingBuffer* rb, const char* data, int length) {
    int space_available = (rb->size - (atomic_load(&rb->head) - atomic_load(&rb->tail) + rb->size) % rb->size) - sizeof(DataHeader);
    if (length > space_available) {
        return false; // Not enough space
    }

    DataHeader hdr = { length };

    int head = atomic_load(&rb->head);
    memcpy(&rb->buffer[head], &hdr, sizeof(DataHeader));
    head = (head + sizeof(DataHeader)) % rb->size;

    for (int i = 0; i < length; i++) {
        rb->buffer[head] = data[i];
        head = (head + 1) % rb->size;
    }

    atomic_store(&rb->head, head); // Update head pointer once
    return true;
}

int ringbuffer_read(RingBuffer* rb, char* data, int length) {
    if (ringbuffer_is_empty(rb)) {
        return 0;
    }

    int tail = atomic_load(&rb->tail);
    DataHeader hdr;
    memcpy(&hdr, &rb->buffer[tail], sizeof(DataHeader));
    tail = (tail + sizeof(DataHeader)) % rb->size;

    int count = (hdr.data_size < length) ? hdr.data_size : length;

    for (int i = 0; i < count; i++) {
        data[i] = rb->buffer[tail];
        tail = (tail + 1) % rb->size;
    }

    atomic_store(&rb->tail, tail); // Update tail pointer once
    return count;
}
