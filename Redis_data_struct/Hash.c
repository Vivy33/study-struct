#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义返回状态
#define DICT_OK 0
#define DICT_ERR 1

// 定义字典条目的结构
struct dictEntry {
    void *key;                // 键
    void *val;                // 值
    struct dictEntry *next;   // 指向下一个条目的指针，用于解决哈希冲突
};

// 定义字典的结构
struct dict {
    struct dictEntry **table; // 哈希表数组
    unsigned long size;       // 哈希表大小
    unsigned long sizemask;   // 大小掩码，用于计算哈希值
    unsigned long used;       // 已使用的条目数量
};

// 简单的哈希函数示例
unsigned int dict_hash_function(const void *key) {
    unsigned int hash = 5381;
    const char *ptr = key;
    while (*ptr) {
        hash = ((hash << 5) + hash) + *ptr++; /* hash * 33 + c */
    }
    return hash;
}

// 创建一个新的字典
struct dict *dict_create(unsigned long size) {
    struct dict *d = malloc(sizeof(*d)); // 分配内存给字典
    if (d == NULL) return NULL; // 检查内存分配是否成功
    d->size = size; // 设置字典大小
    d->sizemask = size - 1; // 设置大小掩码
    d->table = calloc(size, sizeof(struct dictEntry*)); // 分配内存给哈希表
    if (d->table == NULL) {
        free(d); // 如果分配失败，释放字典内存
        return NULL;
    }
    d->used = 0; // 初始化已使用条目数量
    return d;
}

// 向字典中添加条目
int dict_add(struct dict *d, void *key, void *val) {
    unsigned int h = dict_hash_function(key) & d->sizemask; // 计算哈希值并映射到哈希表
    struct dictEntry *entry = malloc(sizeof(*entry)); // 分配内存给新条目
    if (entry == NULL) return DICT_ERR; // 检查内存分配是否成功
    entry->key = strdup(key); // 动态分配并复制键
    entry->val = strdup(val); // 动态分配并复制值
    entry->next = d->table[h]; // 将条目插入到链表的头部
    d->table[h] = entry; // 更新哈希表指针
    d->used++; // 增加已使用条目数量
    return DICT_OK;
}

// 在字典中查找条目
void *dict_find(struct dict *d, const void *key) {
    unsigned int h = dict_hash_function(key) & d->sizemask; // 计算哈希值并映射到哈希表
    struct dictEntry *he = d->table[h]; // 获取哈希表中的链表头
    while (he) {
        if (strcmp(he->key, key) == 0) { // 比较键是否相同
            return he->val; // 如果找到匹配的键，返回对应的值
        }
        he = he->next; // 移动到链表的下一个条目
    }
    return NULL; // 如果没有找到，返回NULL
}

// 释放字典的内存
void dict_free(struct dict *d) {
    unsigned long i;
    for (i = 0; i < d->size; i++) { // 遍历哈希表
        struct dictEntry *he, *nextHe;
        if ((he = d->table[i]) == NULL) continue; // 如果链表为空，继续下一个哈希表槽
        while (he) { // 遍历链表
            nextHe = he->next; // 保存下一个条目
            free(he->key); // 释放键的内存
            free(he->val); // 释放值的内存
            free(he); // 释放当前条目的内存
            he = nextHe; // 移动到下一个条目
        }
    }
    free(d->table); // 释放哈希表的内存
    free(d); // 释放字典结构的内存
}

int main() {
    // 创建一个新的字典
    struct dict *myDict = dict_create(16);

    // 添加一些键值对
    dict_add(myDict, "key1", "value1");
    dict_add(myDict, "key2", "value2");
    dict_add(myDict, "key3", "value3");

    // 查找并打印键值对
    printf("key1: %s\n", (char *)dict_find(myDict, "key1"));
    printf("key2: %s\n", (char *)dict_find(myDict, "key2"));
    printf("key3: %s\n", (char *)dict_find(myDict, "key3"));

    // 释放字典的内存
    dict_free(myDict);

    return 0;
}
