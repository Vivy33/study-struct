#include "ringbuffer.h"
#include <string.h>
#include <stdatomic.h>

bool ringbuffer_is_empty(const RingBuffer* rb) {
    return atomic_load(&rb->head) == atomic_load(&rb->tail);
}

bool ringbuffer_is_full(const RingBuffer* rb) {
    return ((atomic_load(&rb->head) + sizeof(DataHeader)) % rb->size) == atomic_load(&rb->tail);
}

bool ringbuffer_write(RingBuffer* rb, const char* data, int length, int k) {
    if (length > k) {
        return false; // 数据长度超过最大限制
    }

    int space_available = (rb->size - (atomic_load(&rb->head) - atomic_load(&rb->tail) + rb->size) % rb->size) - sizeof(DataHeader);
    if (length > space_available) {
        return false; // 没有足够的空间
    }

    DataHeader hdr = { length };

    int head = atomic_load(&rb->head);
    memcpy(&rb->buffer[head], &hdr, sizeof(DataHeader));
    head = (head + sizeof(DataHeader)) % rb->size;

    for (int i = 0; i < length; i++) {
        rb->buffer[head] = data[i];
        head = (head + 1) % rb->size;
    }

    atomic_store(&rb->head, head); // 一次性更新head指针
    return true;
}

int ringbuffer_read(RingBuffer* rb, char* data, int k) {
    if (ringbuffer_is_empty(rb)) {
        return 0;
    }

    int tail = atomic_load(&rb->tail);
    DataHeader hdr;
    memcpy(&hdr, &rb->buffer[tail], sizeof(DataHeader));

    if (hdr.data_size > k) {
        return -1; // 防止读取超过最大长度的数据
    }

    tail = (tail + sizeof(DataHeader)) % rb->size;

    for (int i = 0; i < hdr.data_size; i++) {
        data[i] = rb->buffer[tail];
        tail = (tail + 1) % rb->size;
    }

    atomic_store(&rb->tail, tail); // 一次性更新tail指针
    return hdr.data_size;
}
