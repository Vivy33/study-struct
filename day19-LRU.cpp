#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/**
 * @struct Node
 * @brief 双向链表节点结构
 *
 * 用于在lru缓存中存储键值对和维护访问顺序。
 */
typedef struct Node {
    int key;            /**< 缓存项的键 */
    int value;          /**< 缓存项的值 */
    struct Node* prev;  /**< 指向前一个节点的指针 */
    struct Node* next;  /**< 指向后一个节点的指针 */
} Node;

/**
 * @struct lruCache
 * @brief lru缓存结构
 *
 * 包含缓存的容量、当前大小、头尾节点和哈希表。
 */
typedef struct {
    int capacity;    /**< 缓存的最大容量 */
    int size;        /**< 当前缓存中的项数 */
    Node* head;      /**< 指向双向链表头部的哨兵节点 */
    Node* tail;      /**< 指向双向链表尾部的哨兵节点 */
    Node** hash;     /**< 用于快速查找的哈希表 */
} lruCache;

/**
 * @brief 创建一个新的节点
 *
 * @param key 节点的键
 * @param value 节点的值
 * @return Node* 返回新创建的节点，如果内存分配失败则返回NULL
 */
Node* createNode(int key, int value) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        return NULL;  // 内存分配失败
    }
    newNode->key = key;
    newNode->value = value;
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

/**
 * @brief 创建一个新的lru缓存
 *
 * @param capacity 缓存的最大容量
 * @return lruCache* 返回新创建的lru缓存，如果创建失败则返回NULL
 */
lruCache* lruCacheCreate(int capacity) {
    if (capacity <= 0) {
        return NULL;  // 无效的容量
    }

    lruCache* cache = (lruCache*)malloc(sizeof(lruCache));
    if (cache == NULL) {
        return NULL;  // 内存分配失败
    }

    cache->capacity = capacity;
    cache->size = 0;
    cache->head = createNode(0, 0);
    cache->tail = createNode(0, 0);

    if (cache->head == NULL || cache->tail == NULL) {
        free(cache);
        return NULL;  // 节点创建失败
    }

    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;

    cache->hash = (Node**)calloc(10001, sizeof(Node*));
    if (cache->hash == NULL) {
        free(cache->head);
        free(cache->tail);
        free(cache);
        return NULL;  // 哈希表创建失败
    }

    return cache;
}

/**
 * @brief 将节点移动到链表头部（最近使用）
 *
 * @param cache lru缓存
 * @param node 要移动的节点
 */
void moveToHead(lruCache* cache, Node* node) {
    // 从原位置删除节点
    node->prev->next = node->next;
    node->next->prev = node->prev;

    // 将节点插入到头部
    node->next = cache->head->next;
    node->prev = cache->head;
    cache->head->next->prev = node;
    cache->head->next = node;
}

/**
 * @brief 将新节点添加到链表头部
 *
 * @param cache lru缓存
 * @param node 要添加的新节点
 */
void addToHead(lruCache* cache, Node* node) {
    node->next = cache->head->next;
    node->prev = cache->head;
    cache->head->next->prev = node;
    cache->head->next = node;
}

/**
 * @brief 移除链表尾部的节点（最久未使用）
 *
 * @param cache lru缓存
 */
void removeTail(lruCache* cache) {
    Node* tail = cache->tail->prev;
    tail->prev->next = cache->tail;
    cache->tail->prev = tail->prev;
    cache->hash[tail->key] = NULL;
    free(tail);
}

/**
 * @brief 从lru缓存中获取值
 *
 * @param obj lru缓存对象
 * @param key 要查找的键
 * @return int 返回找到的值，如果未找到则返回-1
 */
int lruCacheGet(lruCache* obj, int key) {
    if (key < 0 || key > 10000 || obj == NULL) {
        return -1;  // 无效的键或对象
    }

    if (obj->hash[key] == NULL) {
        return -1;  // 键不存在
    }

    Node* node = obj->hash[key];
    moveToHead(obj, node);
    return node->value;
}

/**
 * @brief 向lru缓存中插入或更新值
 *
 * @param obj lru缓存对象
 * @param key 要插入或更新的键
 * @param value 要插入或更新的值
 */
void lruCachePut(lruCache* obj, int key, int value) {
    if (key < 0 || key > 10000 || obj == NULL) {
        return;  // 无效的键或对象
    }

    if (obj->hash[key] != NULL) {
        Node* node = obj->hash[key];
        node->value = value;
        moveToHead(obj, node);
    } else {
        Node* newNode = createNode(key, value);
        if (newNode == NULL) {
            return;  // 内存分配失败
        }

        obj->hash[key] = newNode;
        addToHead(obj, newNode);
        obj->size++;

        if (obj->size > obj->capacity) {
            removeTail(obj);
            obj->size--;
        }
    }
}

/**
 * @brief 释放lru缓存占用的所有内存
 *
 * @param obj 要释放的lru缓存对象
 */
void lruCacheFree(lruCache* obj) {
    if (obj == NULL) {
        return;
    }

    Node* current = obj->head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }
    free(obj->hash);
    free(obj);
}

// 单元测试
void runTests() {
    printf("Running lru Cache tests...\n");

    // Test 1: Basic functionality
    {
        lruCache* cache = lruCacheCreate(2);
        lruCachePut(cache, 1, 1);
        lruCachePut(cache, 2, 2);
        assert(lruCacheGet(cache, 1) == 1);
        lruCachePut(cache, 3, 3);    // 这应该会逐出key 2
        assert(lruCacheGet(cache, 2) == -1);
        lruCachePut(cache, 4, 4);    // 这应该会逐出key 1
        assert(lruCacheGet(cache, 1) == -1);
        assert(lruCacheGet(cache, 3) == 3);
        assert(lruCacheGet(cache, 4) == 4);
        lruCacheFree(cache);
        printf("Test 1 passed.\n");
    }

    // Test 2: Update existing key
    {
        lruCache* cache = lruCacheCreate(2);
        lruCachePut(cache, 1, 1);
        lruCachePut(cache, 2, 2);
        assert(lruCacheGet(cache, 1) == 1);
        lruCachePut(cache, 1, 10);   // 更新已存在的key
        assert(lruCacheGet(cache, 1) == 10);
        assert(lruCacheGet(cache, 2) == 2);
        lruCacheFree(cache);
        printf("Test 2 passed.\n");
    }

    // Test 3: Capacity of 1
    {
        lruCache* cache = lruCacheCreate(1);
        lruCachePut(cache, 1, 1);
        lruCachePut(cache, 2, 2);
        assert(lruCacheGet(cache, 1) == -1);
        assert(lruCacheGet(cache, 2) == 2);
        lruCacheFree(cache);
        printf("Test 3 passed.\n");
    }

    // Test 4: Get non-existent key
    {
        lruCache* cache = lruCacheCreate(3);
        assert(lruCacheGet(cache, 1) == -1);
        lruCachePut(cache, 1, 1);
        assert(lruCacheGet(cache, 2) == -1);
        lruCacheFree(cache);
        printf("Test 4 passed.\n");
    }

    // Test 5: lru order
    {
        lruCache* cache = lruCacheCreate(3);
        lruCachePut(cache, 1, 1);
        lruCachePut(cache, 2, 2);
        lruCachePut(cache, 3, 3);
        lruCacheGet(cache, 1);       // 这应该把1移到最近使用
        lruCachePut(cache, 4, 4);    // 这应该会逐出key 2
        assert(lruCacheGet(cache, 2) == -1);
        assert(lruCacheGet(cache, 1) == 1);
        assert(lruCacheGet(cache, 3) == 3);
        assert(lruCacheGet(cache, 4) == 4);
        lruCacheFree(cache);
        printf("Test 5 passed.\n");
    }

    printf("All tests passed!\n");
}

int main() {
    runTests();
    return 0;
}
