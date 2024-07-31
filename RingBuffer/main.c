#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void process_a();
void process_b();

int main() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // 子进程
        process_b();
    } else {
        // 父进程
        process_a();
        wait(NULL); // 等待子进程完成
    }

    return 0;
}
