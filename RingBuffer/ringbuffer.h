#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>

#define RINGBUFFER_SIZE 1024

typedef struct {
    int head;
    int tail;
    int size;
    char buffer[RINGBUFFER_SIZE];
} RingBuffer;

void ringbuffer_init(RingBuffer* rb);
bool ringbuffer_is_empty(const RingBuffer* rb);
bool ringbuffer_is_full(const RingBuffer* rb);
bool ringbuffer_write(RingBuffer* rb, const char* data, int length);
int ringbuffer_read(RingBuffer* rb, char* data, int length);

#endif // RINGBUFFER_H
