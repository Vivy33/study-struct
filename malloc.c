#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// #define MAX_SIZE 50000000

// 测试特定大小内存块申请释放速度
// 第一个参数是申请内存的大小，第二个参数是测试的次数
// 返回值是总申请时间的耗时,单位是ms
int benchmark_memory_allocate(int size, int count) {

    struct timespec start, end;

    // MONO非精准时间，精度微秒
    // CLOCK_REALTIME 开销高
    // man clock_gettime
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < count; i++) {
        void *p = malloc(size); // 把p缓存下来，不要每次都去释放
        free(p);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long elapsed_time = (end.tv_sec - start.tv_sec) * 1000 + 
                        (end.tv_nsec - start.tv_nsec) / 1000000;

    return elapsed_time;
}

int main(int argc, char *argv[]) {
    int size = 33587201; // 32.031250953674316MB
    int elapsed_time = 0;

    if (argc >= 2) {
        size = 33382401; // 31.835938453674316MB
    }

    elapsed_time = benchmark_memory_allocate(size, 1000000);
    printf("Size: %d bytes, Time: %d ms\n", size, elapsed_time);

    return 0;
}

//  sudo perf record -g ./a.out
//  sudo perf report
//+    9.74%     8.70%  a.out    [kernel.kallsyms]     [k] hyperv_flush_tlb_multi
//+    9.59%     5.43%  a.out    [kernel.kallsyms]     [k] free_pgd_range