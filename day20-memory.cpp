#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>

// 测试参数
#define VECTOR_SIZE 10000
#define ARRAY_SIZE 10000
#define SHARED_MEM_SIZE 100

/**
 * @brief 用于测量时间的结构体
 * 
 * 使用 RDTSC 指令来获取 CPU 时钟周期计数，用于高精度计时
 */
struct Timer {
    unsigned long long start;
    unsigned long long end;
};

/**
 * @brief 开始计时
 * 
 * @param timer 计时器结构体指针
 * 记录当前的 CPU 时钟周期计数作为起始时间
 */
void startTimer(struct Timer* timer) {
    timer->start = __rdtsc();
}

/**
 * @brief 停止计时
 * 
 * @param timer 计时器结构体指针
 * 记录当前的 CPU 时钟周期计数作为结束时间
 */
void stopTimer(struct Timer* timer) {
    timer->end = __rdtsc();
}

/**
 * @brief 获取经过的 CPU 周期数
 * 
 * @param timer 计时器结构体指针
 * @return unsigned long long 返回从开始到结束经过的 CPU 周期数
 */
unsigned long long getElapsedCycles(struct Timer* timer) {
    return timer->end - timer->start;
}

/**
 * @brief 共享内存结构体
 * 
 * 用于在共享内存中存储整数数组
 */
struct SharedMemoryData {
    int data[SHARED_MEM_SIZE];
};

/**
 * @brief 内存分配测试函数
 * 
 * 测试动态数组和普通数组的内存分配性能
 */
void memoryAllocationTest() {
    printf("Memory Allocation Test:\n");

    // 测试动态数组分配内存
    {
        struct Timer timer;
        startTimer(&timer);
        int* vec = (int*)malloc(VECTOR_SIZE * sizeof(int));
        stopTimer(&timer);
        printf("  Dynamic array allocation time (cycles): %llu\n", getElapsedCycles(&timer));
        free(vec);
    }

    // 测试静态数组分配内存
    {
        struct Timer timer;
        startTimer(&timer);
        int arr[ARRAY_SIZE];
        stopTimer(&timer);
        printf("  Static array allocation time (cycles): %llu\n", getElapsedCycles(&timer));
    }
}

/**
 * @brief 共享内存和简单队列测试函数
 * 
 * 测试共享内存的创建、写入和读取性能
 */
void sharedMemoryAndQueueTest() {
    printf("\nShared Memory and Queue Test:\n");

    // 生成唯一的键值
    key_t key = ftok(".", 'a');
    if (key == -1) {
        fprintf(stderr, "  Error: ftok failed. %s\n", strerror(errno));
        return;
    }

    // 创建共享内存段
    // shmget 函数用于创建一个新的共享内存段或获取一个已存在的共享内存段
    // 参数说明：
    //   key: 用于标识共享内存段的键值
    //   sizeof(struct SharedMemoryData): 共享内存段的大小（以字节为单位）
    //   IPC_CREAT | 0666: 标志位，IPC_CREAT表示如果共享内存不存在则创建它，0666设置访问权限
    // 返回值：
    //   成功时返回共享内存标识符（非负整数）
    //   失败时返回-1
    int shmid = shmget(key, sizeof(struct SharedMemoryData), IPC_CREAT | 0666);
    if (shmid == -1) {
        fprintf(stderr, "  Error: shmget failed. %s\n", strerror(errno));
        return;
    }

    // 将共享内存段连接到进程的地址空间
    // shmat 函数用于将共享内存段附加到调用进程的地址空间
    // 参数说明：
    //   shmid: 由shmget返回的共享内存标识符
    //   NULL: 让系统自动选择一个合适的地址来附加共享内存
    //   0: 标志位，这里表示共享内存段是可读可写的
    // 返回值：
    //   成功时返回指向共享内存起始位置的指针
    //   失败时返回(void *)-1
    struct SharedMemoryData* pData = (struct SharedMemoryData*)shmat(shmid, NULL, 0);
    if (pData == (void*)-1) {
        fprintf(stderr, "  Error: shmat failed. %s\n", strerror(errno));
        shmctl(shmid, IPC_RMID, NULL);
        return;
    }

    // 模拟无锁队列操作（向共享内存数组写入数据）
    {
        struct Timer timer;
        startTimer(&timer);
        for (int i = 0; i < SHARED_MEM_SIZE; ++i) {
            pData->data[i] = i;
        }
        stopTimer(&timer);
        printf("  Shared memory write time (cycles): %llu\n", getElapsedCycles(&timer));
    }

    // 读取共享内存数据
    {
        int readData[SHARED_MEM_SIZE];
        struct Timer timer;
        startTimer(&timer);
        for (int i = 0; i < SHARED_MEM_SIZE; ++i) {
            readData[i] = pData->data[i];
        }
        stopTimer(&timer);
        printf("  Shared memory read time (cycles): %llu\n", getElapsedCycles(&timer));
    }

    // 清理资源
    // 分离共享内存段
    if (shmdt(pData) == -1) {
        fprintf(stderr, "  Error: shmdt failed. %s\n", strerror(errno));
    }

    // 删除共享内存段
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        fprintf(stderr, "  Error: shmctl failed. %s\n", strerror(errno));
    }
}

int main() {
    memoryAllocationTest();
    sharedMemoryAndQueueTest();

    return 0;
}
