#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_LEVEL 16

// 跳表节点结构体
struct zskip_listNode {
    double score; // 节点的分数
    char *ele; // 节点的元素
    struct zskip_listNode *forward[MAX_LEVEL]; // 前向指针数组
};

// 跳表结构体
struct zskip_list {
    struct zskip_listNode *header; // 跳表头节点
    int level; // 当前跳表的最大层数
};

// 创建一个新的节点
struct zskip_listNode* create_Node(int level, double score, char *ele) {
    struct zskip_listNode *node = malloc(sizeof(struct zskip_listNode));
    node->score = score;
    node->ele = ele;
    for (int i = 0; i < level; i++) {
        node->forward[i] = NULL; // 初始化前向指针为NULL
    }
    return node;
}

// 创建一个新的跳表
struct zskip_list* create_Skiplist() {
    struct zskip_list *zsl = malloc(sizeof(struct zskip_list));
    zsl->level = 1; // 初始层数为1
    zsl->header = create_Node(MAX_LEVEL, 0, NULL); // 创建头节点
    return zsl;
}

// 随机生成节点的层数
int randomLevel() {
    int level = 1;
    while (rand() % 2 && level < MAX_LEVEL) { // 以50%的概率增加层数
        level++;
    }
    return level;
}

// 插入节点
void insert(struct zskip_list *zsl, double score, char *ele) {
    struct zskip_listNode *update[MAX_LEVEL]; // 记录更新路径
    struct zskip_listNode *x = zsl->header;
    for (int i = zsl->level - 1; i >= 0; i--) { // 从最高层开始
        while (x->forward[i] && x->forward[i]->score < score) {
            x = x->forward[i]; // 向前移动
        }
        update[i] = x; // 记录将要更新的节点
    }
    int level = randomLevel(); // 随机生成新节点的层数
    if (level > zsl->level) {
        for (int i = zsl->level; i < level; i++) {
            update[i] = zsl->header; // 更新需要插入的层数
        }
        zsl->level = level; // 更新跳表的层数
    }
    x = create_Node(level, score, ele); // 创建新节点
    for (int i = 0; i < level; i++) {
        x->forward[i] = update[i]->forward[i]; // 新节点的指针指向原来的节点
        update[i]->forward[i] = x; // 更新原来节点的指针指向新节点
    }
}

// 查找节点
struct zskip_listNode* find(struct zskip_list *zsl, double score) {
    struct zskip_listNode *x = zsl->header;
    for (int i = zsl->level - 1; i >= 0; i--) {
        while (x->forward[i] && x->forward[i]->score < score) {
            x = x->forward[i];
        }
    }
    x = x->forward[0];
    if (x && x->score == score) {
        return x; // 找到节点
    }
    return NULL; // 没有找到节点
}

// 打印跳表
void print_Skiplist(struct zskip_list *zsl) {
    for (int i = 0; i < zsl->level; i++) {
        struct zskip_listNode *x = zsl->header->forward[i];
        printf("Level %d: ", i);
        while (x) {
            printf("(%f, %s) ", x->score, x->ele);
            x = x->forward[i];
        }
        printf("\n");
    }
}

int main() {
    srand(time(NULL)); // 初始化随机数种子
    struct zskip_list *zsl = create_Skiplist();
    
    // 插入几个节点
    insert(zsl, 1.0, "one");
    insert(zsl, 2.0, "two");
    insert(zsl, 3.0, "three");
    insert(zsl, 0.5, "half");
    
    print_Skiplist(zsl); // 打印跳表
    
    struct zskip_listNode *node = find(zsl, 2.0); // 查找节点
    if (node) {
        printf("Found: %f, %s\n", node->score, node->ele);
    } else {
        printf("Not found\n");
    }
    
    free(zsl);
    return 0;
}
