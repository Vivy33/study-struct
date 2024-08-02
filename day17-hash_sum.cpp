/*给定一个整数数组 nums 和一个整数目标值 target，请你在该数组中找出 和为目标值 target  的那 两个 整数，并返回它们的数组下标。

你可以假设每种输入只会对应一个答案。但是，数组中同一个元素在答案里不能重复出现。

你可以按任意顺序返回答案。

 

示例 1：

输入：nums = [2,7,11,15], target = 9
输出：[0,1]
解释：因为 nums[0] + nums[1] == 9 ，返回 [0, 1] 。

进阶：你可以想出一个时间复杂度小于 O(n2) 的算法吗？*/

#include <stdio.h>
#include <stdlib.h>

// 定义哈希表的结构
typedef struct {
    int key;
    int value;
} HashItem;

// 定义哈希表的大小
#define HASH_SIZE 2048

// 创建哈希表
HashItem* createHashTable() {
    HashItem* hashTable = (HashItem*)malloc(sizeof(HashItem) * HASH_SIZE);
    for (int i = 0; i < HASH_SIZE; i++) {
        hashTable[i].key = -1;
        hashTable[i].value = -1;
    }
    return hashTable;
}

// 哈希函数
int hash(int key) {
    return abs(key) % HASH_SIZE;
}

// 在哈希表中插入元素
void insert(HashItem* hashTable, int key, int value) {
    int index = hash(key);
    while (hashTable[index].key != -1) {
        index = (index + 1) % HASH_SIZE;
    }
    hashTable[index].key = key;
    hashTable[index].value = value;
}

// 从哈希表中查找元素
int search(HashItem* hashTable, int key) {
    int index = hash(key);
    while (hashTable[index].key != -1) {
        if (hashTable[index].key == key) {
            return hashTable[index].value;
        }
        index = (index + 1) % HASH_SIZE;
    }
    return -1;
}

int* twoSum(const int* nums, int numsSize, const int target, int* returnSize) {
    HashItem* hashTable = createHashTable();
    int* result = (int*)malloc(2 * sizeof(int));
    
    for (int i = 0; i < numsSize; i++) {
        printf("Checking element nums[%d] = %d\n", i, nums[i]);
        int complement = target - nums[i];
        int complementIndex = search(hashTable, complement);
        if (complementIndex != -1) {
            result[0] = complementIndex;
            result[1] = i;
            *returnSize = 2;
            free(hashTable);
            return result;
        }
        insert(hashTable, nums[i], i);
        printf("Inserted nums[%d] = %d into hashTable\n", i, nums[i]);
    }

    *returnSize = 0;
    free(hashTable);
    return NULL;
}

int main() {
    int nums[] = {2, 7, 11, 15};
    int target = 9;
    int returnSize;
    int* result = twoSum(nums, 4, target, &returnSize);

    if (result) {
        printf("Indices: [%d, %d]\n", result[0], result[1]);
        free(result);
    } else {
        printf("No solution found\n");
    }

    return 0;
}
