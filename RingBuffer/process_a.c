#include "shared_memory.h"
#include "ringbuffer.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MESSAGE_SIZE 100

void process_a(RingBuffer* rb_x, RingBuffer* rb_y) {
    const char* msg = "Hello from A!";
    printf("Process A: Writing to shared memory X\n");
    ringbuffer_write(rb_x, msg, strlen(msg));

    char buffer[MESSAGE_SIZE];
    while (ringbuffer_is_empty(rb_y)) {
        usleep(1000); // Wait for data from B
    }

    int bytes_read = ringbuffer_read(rb_y, buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';
    printf("Process A: Read from shared memory Y: %s\n", buffer);
}
