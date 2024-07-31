/*给你一个链表的头节点 head ，判断链表中是否有环。

如果链表中有某个节点，可以通过连续跟踪 next 指针再次到达，则链表中存在环。 为了表示给定链表中的环，评测系统内部使用整数 pos 来表示链表尾连接到链表中的位置（索引从 0 开始）。
注意：pos 不作为参数进行传递。仅仅是为了标识链表的实际情况。

如果链表中存在环 ，则返回 true 。 否则，返回 false 。

输入：head = [3,2,0,-4], pos = 1
输出：true
解释：链表中有一个环，其尾部连接到第二个节点。*/

//思路：可以通过快慢指针（Floyd判圈算法）来判断链表中是否存在环。
//快指针每次移动两个节点，慢指针每次移动一个节点。如果链表中存在环，快慢指针最终会相遇。如果链表中没有环，快指针将到达链表末尾。

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// 定义链表节点结构
struct ListNode {
    int val;
    struct ListNode *next;
};

// 创建一个新的链表节点
struct ListNode* createNode(int val) {
    struct ListNode* node = (struct ListNode*)malloc(sizeof(struct ListNode));
    if (node == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    node->val = val;
    node->next = NULL;
    return node;
}

// 判断链表是否有环，并返回环节点。如果没有环，返回NULL。
struct ListNode* detectCycle(struct ListNode *head) {
    if (head == NULL) {
        printf("The list is empty.\n");
        return NULL;
    }

    struct ListNode *slow = head;
    struct ListNode *fast = head;

    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;

        printf("Visiting nodes - slow: %d, fast: %d\n", slow->val, fast->val);

        if (slow == fast) {
            printf("Cycle detected at node with value %d\n", slow->val);
            return slow;
        }
    }

    printf("No cycle detected.\n");
    return NULL;
}

// 断开链表中的环
void removeCycle(struct ListNode *head, struct ListNode *cycleNode) {
    struct ListNode *ptr1 = head;
    struct ListNode *ptr2 = cycleNode;

    // 找到环的入口点
    while (ptr1 != ptr2) {
        ptr1 = ptr1->next;
        ptr2 = ptr2->next;
    }

    // 找到环入口后，找环的最后一个节点并断开环
    struct ListNode *prev = NULL;
    while (ptr2->next != ptr1) {
        prev = ptr2;
        ptr2 = ptr2->next;
    }
    prev->next = NULL;

    printf("Cycle removed. Node with value %d no longer points to %d\n", prev->val, ptr2->val);
}

// 释放链表的内存
void freeList(struct ListNode *head) {
    while (head != NULL) {
        struct ListNode *temp = head;
        printf("Freeing node with value %d\n", head->val);
        head = head->next;
        free(temp);
    }
}

// 主函数进行测试
int main() {
    // 构建测试链表 [3,2,0,-4] 并制造环 pos = 1
    struct ListNode* head = createNode(3);
    head->next = createNode(2);
    head->next->next = createNode(0);
    head->next->next->next = createNode(-4);
    head->next->next->next->next = head->next; // 制造环

    // 检查链表是否有环
    printf("Checking if the list has a cycle...\n");
    struct ListNode* cycleNode = detectCycle(head);
    if (cycleNode != NULL) {
        // 断开链表中的环
        removeCycle(head, cycleNode);
    }

    // 释放链表内存
    printf("Freeing the list...\n");
    freeList(head);

    return 0;
}


//PS:由于链表有环，如果直接调用 freeList 函数释放内存会导致无限循环。要正确释放带环的链表内存，需要在断开环后再释放内存。
