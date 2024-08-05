#include "shared_memory.h"
#include "ringbuffer.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MESSAGE_SIZE 100

void process_b(RingBuffer* rb_x, RingBuffer* rb_y) {
    char buffer[MESSAGE_SIZE];
    while (ringbuffer_is_empty(rb_x)) {
        usleep(1000); // Wait for data from A
    }

    int bytes_read = ringbuffer_read(rb_x, buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';
    printf("Process B: Read from shared memory X: %s\n", buffer);

    const char* msg = "Hello from B!";
    printf("Process B: Writing to shared memory Y\n");
    ringbuffer_write(rb_y, msg, strlen(msg));
}
