#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "shared_memory.h"
#include "ringbuffer.h"

void process_a(RingBuffer* rb_x, RingBuffer* rb_y);
void process_b(RingBuffer* rb_x, RingBuffer* rb_y);

int main() {
    RingBuffer* shm_x = create_anonymous_shared_memory(sizeof(RingBuffer));
    RingBuffer* shm_y = create_anonymous_shared_memory(sizeof(RingBuffer));

    ringbuffer_init(shm_x);
    ringbuffer_init(shm_y);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // 子进程
        process_b(shm_x, shm_y);
    } else {
        // 父进程
        process_a(shm_x, shm_y);
        wait(NULL); // 等待子进程完成
    }

    // 解除共享内存映射
    munmap(shm_x, sizeof(RingBuffer));
    munmap(shm_y, sizeof(RingBuffer));

    return 0;
}
