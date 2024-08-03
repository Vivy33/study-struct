#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>
#include <pthread.h>

#define RINGBUFFER_SIZE 1024

typedef struct {
    int head;
    int tail;
    int size;
} DataHead;

typedef struct {
    DataHead head;
    char buffer[RINGBUFFER_SIZE];
    pthread_mutex_t mutex;  // 确保这里定义了互斥锁
} RingBuffer;

void ringbuffer_init(RingBuffer* rb);
bool ringbuffer_is_empty(const RingBuffer* rb);
bool ringbuffer_is_full(const RingBuffer* rb);
bool ringbuffer_write(RingBuffer* rb, const char* data, int length);
int ringbuffer_read(RingBuffer* rb, char* data, int length);

#endif // RINGBUFFER_H
