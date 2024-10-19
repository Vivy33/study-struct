#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义一个 SDS（Simple Dynamic String，简单动态字符串）结构体
struct sds {
    int len;    // 当前字符串的长度
    int avail;  // 缓冲区中可用的空间
    char buf[]; // 用于实际字符存储的灵活数组成员
};

// 函数：从一个初始的 C 字符串创建一个新的 SDS
struct sds* sds_new(const char *init) {
    size_t initlen;
    if (init == NULL) {
        initlen = 0; // 如果初始化字符串为 NULL，则长度为 0
    } else {
        initlen = strlen(init); // 否则，计算初始字符串的长度
    }
    
    // 为 sds 结构体以及字符串和空终止符分配所需的内存
    struct sds* sh = malloc(sizeof(struct sds) + initlen + 1);
    if (sh == NULL) return NULL; // 检查内存分配是否成功
    sh->len = initlen;           // 设置字符串的长度
    sh->avail = 0;               // 初始化可用空间为 0
    if (initlen) {
        memcpy(sh->buf, init, initlen); // 将初始字符串复制到缓冲区
    }
    sh->buf[initlen] = '\0';     // 字符串以空终止符结束
    return sh;                   // 返回新创建的 sds
}

// 释放为 SDS 分配的内存
void sds_free(struct sds *s) {
    if (s == NULL) return; // 检查指针是否为空
    free(s);               // 释放分配的内存
}

// 打印 SDS 的内容
void sds_print(struct sds *s) {
    if (s == NULL) return; // 检查指针是否为空
    printf("%s\n", s->buf); // 打印缓冲区中存储的字符串
}

int main() {
    struct sds *mystr = sds_new("Hello, SDS!");
    if (mystr != NULL) {
        sds_print(mystr); 
        sds_free(mystr);  
    } else {
        printf("Memory allocation failed\n");
    }

    free(mystr);
    return 0;
}
