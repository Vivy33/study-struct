#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义返回状态
#define SET_OK 0
#define SET_ERR 1

// 定义集合条目的结构
struct setEntry {
    void *element;            // 集合中的元素
    struct setEntry *next;    // 指向下一个条目的指针，用于解决哈希冲突
};

// 定义集合的结构
struct set {
    struct setEntry **table;  // 哈希表数组
    unsigned long size;       // 哈希表大小
    unsigned long sizemask;   // 大小掩码，用于计算哈希值
    unsigned long used;       // 已使用的条目数量
};

// 简单的哈希函数示例
unsigned int set_hash_function(const void *element) {
    unsigned int hash = 5381;
    const char *ptr = element;
    while (*ptr) {
        hash = ((hash << 5) + hash) + *ptr++; /* hash * 33 + c */
    }
    return hash;
}

// 创建一个新的集合
struct set *set_create(unsigned long size) {
    struct set *s = malloc(sizeof(*s)); // 分配内存给集合
    if (s == NULL) return NULL; // 检查内存分配是否成功
    s->size = size; // 设置集合大小
    s->sizemask = size - 1; // 设置大小掩码
    s->table = calloc(size, sizeof(struct setEntry*)); // 分配内存给哈希表
    if (s->table == NULL) {
        free(s); // 如果分配失败，释放集合内存
        return NULL;
    }
    s->used = 0; // 初始化已使用条目数量
    return s;
}

// 向集合中添加元素
int set_add(struct set *s, void *element) {
    unsigned int h = set_hash_function(element) & s->sizemask; // 计算哈希值并映射到哈希表
    struct setEntry *entry = s->table[h];

    // 检查元素是否已经存在
    while (entry) {
        if (strcmp(entry->element, element) == 0) {
            return SET_ERR; // 元素已存在
        }
        entry = entry->next;
    }

    // 添加新元素
    entry = malloc(sizeof(*entry)); // 分配内存给新条目
    if (entry == NULL) return SET_ERR; // 检查内存分配是否成功
    entry->element = strdup(element); // 复制元素内容
    entry->next = s->table[h]; // 将条目插入到链表的头部
    s->table[h] = entry; // 更新哈希表指针
    s->used++; // 增加已使用条目数量
    return SET_OK;
}

// 检查元素是否在集合中
int set_member(struct set *s, const void *element) {
    unsigned int h = set_hash_function(element) & s->sizemask; // 计算哈希值并映射到哈希表
    struct setEntry *entry = s->table[h];
    while (entry) {
        if (strcmp(entry->element, element) == 0) {
            return 1; // 元素存在
        }
        entry = entry->next; // 移动到链表的下一个条目
    }
    return 0; // 元素不存在
}

// 释放集合的内存
void set_free(struct set *s) {
    unsigned long i;
    for (i = 0; i < s->size && s->used > 0; i++) { // 遍历哈希表
        struct setEntry *entry, *nextEntry;
        if ((entry = s->table[i]) == NULL) continue; // 如果链表为空，继续下一个哈希表槽
        while (entry) { // 遍历链表
            nextEntry = entry->next; // 保存下一个条目
            free(entry->element); // 释放元素内存
            free(entry); // 释放当前条目的内存
            entry = nextEntry; // 移动到下一个条目
            s->used--; // 减少已使用条目数量
        }
    }
    free(s->table); // 释放哈希表的内存
    free(s); // 释放集合结构的内存
}

int main() {
    // 创建一个新的集合
    struct set *mySet = set_create(16);

    set_add(mySet, "element1");
    set_add(mySet, "element2");
    set_add(mySet, "element3");

    printf("element1 is %s\n", set_member(mySet, "element1") ? "in the set" : "not in the set");
    printf("element2 is %s\n", set_member(mySet, "element2") ? "in the set" : "not in the set");
    printf("element4 is %s\n", set_member(mySet, "element4") ? "in the set" : "not in the set");

    // 释放集合的内存
    set_free(mySet);

    return 0;
}
