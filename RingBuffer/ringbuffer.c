#include "ringbuffer.h"
#include <string.h>
#include <pthread.h>

void ringbuffer_init(RingBuffer* rb) {
    rb->head.head = 0;
    rb->head.tail = 0;
    rb->head.size = RINGBUFFER_SIZE;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&rb->mutex, &attr);
    memset(rb->buffer, 0, RINGBUFFER_SIZE);
}

bool ringbuffer_is_empty(const RingBuffer* rb) {
    pthread_mutex_lock((pthread_mutex_t*)&rb->mutex);
    bool empty = rb->head.head == rb->head.tail;
    pthread_mutex_unlock((pthread_mutex_t*)&rb->mutex);
    return empty;
}

bool ringbuffer_is_full(const RingBuffer* rb) {
    pthread_mutex_lock((pthread_mutex_t*)&rb->mutex);
    bool full = ((rb->head.head + 1) % rb->head.size) == rb->head.tail;
    pthread_mutex_unlock((pthread_mutex_t*)&rb->mutex);
    return full;
}

bool ringbuffer_write(RingBuffer* rb, const char* data, int length) {
    pthread_mutex_lock(&rb->mutex);
    if (length > rb->head.size - 1 || ringbuffer_is_full(rb)) {
        pthread_mutex_unlock(&rb->mutex);
        return false;
    }
    for (int i = 0; i < length; i++) {
        rb->buffer[rb->head.head] = data[i];
        rb->head.head = (rb->head.head + 1) % rb->head.size;
    }
    pthread_mutex_unlock(&rb->mutex);
    return true;
}

int ringbuffer_read(RingBuffer* rb, char* data, int length) {
    pthread_mutex_lock(&rb->mutex);
    if (ringbuffer_is_empty(rb)) {
        pthread_mutex_unlock(&rb->mutex);
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
    pthread_mutex_unlock(&rb->mutex);
    return count;
}
