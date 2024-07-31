#include "shared_memory.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SHM_NAME_X "/shm_x"
#define SHM_NAME_Y "/shm_y"
#define MESSAGE_SIZE 100

void process_a() {
    const int shm_fd_x = create_shared_memory(SHM_NAME_X, sizeof(RingBuffer));
    RingBuffer* const rb_x = map_shared_memory(shm_fd_x, sizeof(RingBuffer));
    ringbuffer_init(rb_x);

    const int shm_fd_y = create_shared_memory(SHM_NAME_Y, sizeof(RingBuffer));
    RingBuffer* const rb_y = map_shared_memory(shm_fd_y, sizeof(RingBuffer));
    ringbuffer_init(rb_y);

    const char* const msg = "Hello from A!";
    printf("Process A: Writing to shared memory X\n");
    ringbuffer_write(rb_x, msg, strlen(msg));

    char buffer[MESSAGE_SIZE];
    while (ringbuffer_is_empty(rb_y)) {
        usleep(1000); // Wait for data from B
    }

    const int bytes_read = ringbuffer_read(rb_y, buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';
    printf("Process A: Read from shared memory Y: %s\n", buffer);

    shm_unlink(SHM_NAME_X);
    shm_unlink(SHM_NAME_Y);
}
