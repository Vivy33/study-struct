//读写文件，统计单词，空格分词个数

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define NUM_THREADS 4   // 定义线程数量
#define CHUNK_SIZE 4096  // 定义每个线程处理的块大小

typedef struct {
    FILE *file;  // 指向文件的指针
    long offset;  // 文件偏移量
    int length;  // 要读取的长度
    int word_count;  // 记录单词数量
} ThreadData;

pthread_mutex_t lock;  // 定义互斥锁

// 线程执行的函数，计算文本中的单词数量
void *count_words(void *arg) {
    ThreadData *data = (ThreadData *)arg;  // 将传入的参数转换为 ThreadData 类型
    FILE *file = data->file;  // 获取文件指针
    long offset = data->offset;  // 获取文件偏移量
    int length = data->length;  // 获取要读取的长度
    int count = 0;  // 初始化单词计数
    int in_word = 0;  // 标志是否在单词中

    char *buffer = (char *)malloc(length + 1);  // 为读取的内容分配内存
    if (buffer == NULL) {
        perror("Error allocating memory");
        return NULL;  // 内存分配失败，退出线程
    }

    pthread_mutex_lock(&lock);  // 加锁
    fseek(file, offset, SEEK_SET);  // 将文件指针移动到指定偏移量
    size_t read_size = fread(buffer, 1, length, file);  // 读取文件内容到缓冲区
    pthread_mutex_unlock(&lock);  // 解锁

    buffer[read_size] = '\0';  // 确保文本以空字符结尾

    // 遍历文本，计算单词数量
    for (int i = 0; i < read_size; i++) {
        if (buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t' || buffer[i] == '\0') {
            if (in_word) {
                count++;  // 如果之前在单词中，现在遇到空白字符，单词计数加1
                in_word = 0;  // 重置 in_word 标志
            }
        } else {
            in_word = 1;  // 如果遇到非空白字符，设置 in_word 标志
        }
    }

    if (in_word) {
        count++;  // 如果最后一个字符是非空白字符，单词计数加1
    }

    data->word_count = count;  // 将计算出的单词数量保存到数据结构中

    free(buffer);  // 释放分配的内存

    return NULL;  // 函数自然退出
}

int main() {
    printf("Opening file...\n");
    FILE *file = fopen("english.txt", "r");  // 打开文件
    if (file == NULL) {
        perror("Error opening file");
        abort();  // 如果打开文件失败，终止程序
    }

    printf("File opened successfully.\n");

    fseek(file, 0, SEEK_END);  // 将文件指针移动到文件末尾
    long file_size = ftell(file);  // 获取文件大小
    fseek(file, 0, SEEK_SET);  // 将文件指针重新指向文件开头

    printf("File size: %ld bytes\n", file_size);

    pthread_t threads[NUM_THREADS];  // 线程标识符数组
    ThreadData thread_data[NUM_THREADS];  // 线程数据结构数组

    pthread_mutex_init(&lock, NULL);  // 初始化互斥锁

    int chunk_size = file_size / NUM_THREADS;  // 计算每个线程处理的文本块大小
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("Creating thread %d...\n", i);
        thread_data[i].file = file;  // 设置文件指针
        thread_data[i].offset = i * chunk_size;  // 设置文件偏移量
        thread_data[i].length = (i == NUM_THREADS - 1) ? file_size - i * chunk_size : chunk_size;  // 设置文本块长度
        thread_data[i].word_count = 0;  // 初始化单词计数
        int ret = pthread_create(&threads[i], NULL, count_words, &thread_data[i]);  // 创建线程
        if (ret != 0) {
            perror("Error creating thread");
            fclose(file);
            abort();  // 如果创建线程失败，终止程序
        }
        printf("Thread %d created successfully.\n", i);
    }

    int total_word_count = 0;  // 初始化总单词计数
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("Joining thread %d...\n", i);
        pthread_join(threads[i], NULL);  // 等待线程结束
        printf("Thread %d joined. Word count: %d\n", i, thread_data[i].word_count);  // 输出每个线程的单词计数结果
        total_word_count += thread_data[i].word_count;  // 累加总单词计数
    }

    pthread_mutex_destroy(&lock);  // 销毁互斥锁
    fclose(file);  // 关闭文件

    printf("Total word count: %d\n", total_word_count);  // 输出总单词计数结果

    return 0;
}
