//读写文件，统计单词，空格分词个数

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <ctype.h>  // 包含 isspace 函数

#define NUM_THREADS 8  // 定义线程数量，视情况可增加
#define READ_SIZE 1048576  // 每次读取的块大小 1MB
#define LOOKAHEAD_SIZE 1024  // 额外读取的字节数以确保完整单词

typedef struct {
    int fd;             // 文件描述符
    off_t offset;       // 文件偏移量
    size_t length;      // 要读取的长度
    atomic_int word_count;  // 记录单词数量（使用原子类型）
} ThreadData;

// 线程执行的函数，计算文本中的单词数量
void *count_words(void *arg) {
    ThreadData *data = (ThreadData *)arg;  // 将传入的参数转换为 ThreadData 类型
    int fd = data->fd;                     // 获取文件描述符
    off_t offset = data->offset;           // 获取文件偏移量
    size_t length = data->length;          // 获取要读取的长度
    int count = 0;                         // 初始化单词计数
    int in_word = 0;                       // 标志是否在单词中

    size_t buffer_size = READ_SIZE + LOOKAHEAD_SIZE + 1;
    char *buffer = (char *)malloc(buffer_size);  // 为读取的内容分配内存
    if (buffer == NULL) {
        perror("Error allocating memory");
        return NULL;  // 内存分配失败，退出线程
    }

    while (length > 0) {
        size_t to_read = length > READ_SIZE ? READ_SIZE : length;

        ssize_t read_size = pread(fd, buffer, to_read + LOOKAHEAD_SIZE, offset);  // 从指定偏移量读取文件内容到缓冲区

        if (read_size < 0) {
            perror("Error reading file");
            free(buffer);
            return NULL;  // 读取失败，退出线程
        }

        // 如果读取到的字节数少于请求的字节数，说明已到文件末尾
        if (read_size < to_read + LOOKAHEAD_SIZE) {
            buffer[read_size] = '\0';  // 确保文本以空字符结尾
        } else {
            // 向前或向后查看，确保在单词边界处结束
            ssize_t lookahead = LOOKAHEAD_SIZE;
            while (lookahead > 0 && !isspace(buffer[to_read + lookahead - 1])) {
                lookahead--;
            }
            buffer[to_read + lookahead] = '\0';  // 确保在单词边界处结束
            read_size = to_read + lookahead;
        }

        // 遍历文本，计算单词数量
        for (ssize_t i = 0; i < read_size; i++) {
            if (isspace(buffer[i])) {
                if (in_word) {
                    count++;  // 如果之前在单词中，现在遇到空白字符，单词计数加1
                    in_word = 0;  // 重置 in_word 标志
                }
            } else {
                in_word = 1;  // 如果遇到非空白字符，设置 in_word 标志
            }
        }

        offset += read_size;  // 更新偏移量
        length -= read_size;  // 更新剩余读取长度
    }

    if (in_word) {
        count++;  // 如果最后一个字符是非空白字符，单词计数加1
    }

    atomic_store(&data->word_count, count);  // 使用原子操作存储单词计数

    free(buffer);  // 释放内存
    return NULL;  // 函数自然退出
}

int main() {
    printf("Opening file...\n");
    int fd = open("english.txt", O_RDONLY);  // 打开文件
    if (fd < 0) {
        perror("Error opening file");
        abort();  // 如果打开文件失败，终止程序
    }

    printf("File opened successfully.\n");

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Error getting file size");
        close(fd);
        abort();  // 如果获取文件大小失败，终止程序
    }
    off_t file_size = st.st_size;  // 获取文件大小

    printf("File size: %ld bytes\n", file_size);

    pthread_t threads[NUM_THREADS];  // 线程标识符数组
    ThreadData thread_data[NUM_THREADS];  // 线程数据结构数组

    size_t chunk_size = file_size / NUM_THREADS;  // 计算每个线程处理的文本块大小
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("Creating thread %d...\n", i);
        thread_data[i].fd = fd;  // 设置文件描述符
        thread_data[i].offset = i * chunk_size;  // 设置文件偏移量
        thread_data[i].length = (i == NUM_THREADS - 1) ? file_size - i * chunk_size : chunk_size;  // 设置文本块长度
        atomic_init(&thread_data[i].word_count, 0);  // 初始化单词计数（使用原子操作）
        int ret = pthread_create(&threads[i], NULL, count_words, &thread_data[i]);  // 创建线程
        if (ret != 0) {
            perror("Error creating thread");
            close(fd);
            abort();  // 如果创建线程失败，终止程序
        }
        printf("Thread %d created successfully.\n", i);
    }

    int total_word_count = 0;  // 初始化总单词计数
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("Joining thread %d...\n", i);
        pthread_join(threads[i], NULL);  // 等待线程结束
        printf("Thread %d joined. Word count: %d\n", i, atomic_load(&thread_data[i].word_count));  // 输出每个线程的单词计数结果
        total_word_count += atomic_load(&thread_data[i].word_count);  // 累加总单词计数
    }

    close(fd);  // 关闭文件

    printf("Total word count: %d\n", total_word_count);  // 输出总单词计数结果

    return 0;
}
