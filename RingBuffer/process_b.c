#include "shared_memory.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SHM_NAME_X "/shm_x"
#define SHM_NAME_Y "/shm_y"
#define MESSAGE_SIZE 100

void process_b() {
    int shm_fd_x = create_shared_memory(SHM_NAME_X, sizeof(RingBuffer));
    RingBuffer* rb_x = map_shared_memory(shm_fd_x, sizeof(RingBuffer));

    int shm_fd_y = create_shared_memory(SHM_NAME_Y, sizeof(RingBuffer));
    RingBuffer* rb_y = map_shared_memory(shm_fd_y, sizeof(RingBuffer));

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
