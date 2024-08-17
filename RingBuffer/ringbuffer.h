#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>
#include <stdatomic.h>

#define RINGBUFFER_SIZE 1024

typedef struct {
    char buffer[RINGBUFFER_SIZE];
    atomic_int head;
    atomic_int tail;
    int size;
} RingBuffer;

typedef struct {
    int data_size;
} DataHeader;

void ringbuffer_init(RingBuffer* rb);
bool ringbuffer_is_empty(const RingBuffer* rb);
bool ringbuffer_is_full(const RingBuffer* rb);
bool ringbuffer_write(RingBuffer* rb, const char* data, int length);
int ringbuffer_read(RingBuffer* rb, char* data, int length);

#endif // RINGBUFFER_H
