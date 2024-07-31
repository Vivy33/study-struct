#include "ringbuffer.h"
#include <string.h>

void ringbuffer_init(RingBuffer* const rb) {
    rb->head.head = 0;
    rb->head.tail = 0;
    rb->head.size = RINGBUFFER_SIZE;
    memset(rb->buffer, 0, RINGBUFFER_SIZE);
}

bool ringbuffer_is_empty(const RingBuffer* const rb) {
    return rb->head.head == rb->head.tail;
}

bool ringbuffer_is_full(const RingBuffer* const rb) {
    return ((rb->head.head + 1) % rb->head.size) == rb->head.tail;
}

bool ringbuffer_write(RingBuffer* const rb, const char* const data, const int length) {
    if (length > rb->head.size - 1) {
        return false;
    }
    for (int i = 0; i < length; i++) {
        if (ringbuffer_is_full(rb)) {
            return false;
        }
        rb->buffer[rb->head.head] = data[i];
        rb->head.head = (rb->head.head + 1) % rb->head.size;
    }
    return true;
}

int ringbuffer_read(RingBuffer* const rb, char* const data, const int length) {
    if (ringbuffer_is_empty(rb)) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < length; i++) {
        if (ringbuffer_is_empty(rb)) {
            break;
        }
        data[i] = rb->buffer[rb->head.tail];
        rb->head.tail = (rb->head.tail + 1) % rb->head.size;
        count++;
    }
    return count;
}
