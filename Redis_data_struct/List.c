#include <stdio.h>
#include <stdlib.h>

// 定义双向链表的节点结构
struct listNode {
    struct listNode *prev; // 指向前一个节点
    struct listNode *next; // 指向后一个节点
    void *value;           // 节点存储的值
};

// 定义双向链表的结构
struct list {
    struct listNode *head; // 指向链表的头节点
    struct listNode *tail; // 指向链表的尾节点
    unsigned long len;     // 链表的长度（节点数量）
};

// 创建一个新的链表，并初始化其成员
struct list* list_create(void) {
    struct list* list = malloc(sizeof(struct list)); // 分配内存给链表结构
    if (list == NULL) return NULL; // 检查内存分配是否成功
    list->head = list->tail = NULL; // 初始化头尾指针为空
    list->len = 0; // 初始化链表长度为0
    return list;
}

// 在链表头部添加一个新节点
void list_add_Node_head(struct list *list, void *value) {
    struct listNode *node = malloc(sizeof(struct listNode)); // 分配内存给新节点
    if (node == NULL) return; // 检查内存分配是否成功
    node->value = value; // 设置节点的值
    if (list->len == 0) { // 如果链表为空
        list->head = list->tail = node; // 新节点是唯一的节点，头尾指针都指向它
        node->prev = node->next = NULL; // 新节点的前后指针都为空
    } else {
        node->prev = NULL; // 新节点的前指针为空
        node->next = list->head; // 新节点的后指针指向当前的头节点
        list->head->prev = node; // 当前头节点的前指针指向新节点
        list->head = node; // 头指针指向新节点
    }
    list->len++; // 链表长度增加
}

// 在链表尾部添加一个新节点
void list_add_Node_tail(struct list *list, void *value) {
    struct listNode *node = malloc(sizeof(struct listNode)); // 分配内存给新节点
    if (node == NULL) return; // 检查内存分配是否成功
    node->value = value; // 设置节点的值
    if (list->len == 0) { // 如果链表为空
        list->head = list->tail = node; // 新节点是唯一的节点，头尾指针都指向它
        node->prev = node->next = NULL; // 新节点的前后指针都为空
    } else {
        node->prev = list->tail; // 新节点的前指针指向当前的尾节点
        node->next = NULL; // 新节点的后指针为空
        list->tail->next = node; // 当前尾节点的后指针指向新节点
        list->tail = node; // 尾指针指向新节点
    }
    list->len++; // 链表长度增加
}

// 从链表中删除一个节点
void list_del_Node(struct list *list, struct listNode *node) {
    if (node->prev) { // 如果节点有前驱节点
        node->prev->next = node->next; // 前驱节点的后指针指向节点的后驱
    } else { // 如果节点是头节点
        list->head = node->next; // 头指针指向节点的后驱
    }
    if (node->next) { // 如果节点有后驱节点
        node->next->prev = node->prev; // 后驱节点的前指针指向节点的前驱
    } else { // 如果节点是尾节点
        list->tail = node->prev; // 尾指针指向节点的前驱
    }
    free(node); // 释放节点的内存
    list->len--; // 链表长度减少
}

// 释放整个链表的内存
void list_free(struct list *list) {
    unsigned long len = list->len; // 获取链表长度
    struct listNode *current = list->head; // 开始遍历的节点为头节点
    struct listNode *next;
    while (len--) { // 遍历整个链表
        next = current->next; // 保存下一个节点
        free(current); // 释放当前节点的内存
        current = next; // 移动到下一个节点
    }
    free(list); // 释放链表结构的内存
}

int main() {
    struct list *mylist = list_create(); // 创建一个新的链表
    list_add_Node_head(mylist, "First"); // 在链表头部添加节点
    list_add_Node_tail(mylist, "Second"); // 在链表尾部添加节点
    list_add_Node_head(mylist, "Zero"); // 在链表头部再添加一个节点
    
    struct listNode *node = mylist->head;
    while (node != NULL) { // 遍历链表并打印节点的值
        printf("%s\n", (char *)node->value);
        node = node->next;
    }
    
    list_free(mylist); // 释放链表的内存
    return 0;
}
