/*给你一个整数数组 nums 。如果任一值在数组中出现 至少两次 ，返回 true ；如果数组中每个元素互不相同，返回 false 。
 

示例 1：

输入：nums = [1,2,3,1]
输出：true
示例 2：

输入：nums = [1,2,3,4]
输出：false
示例 3：

输入：nums = [1,1,1,3,3,4,3,2,4,2]
输出：true
 

提示：

1 <= nums.length <= 105
-109 <= nums[i] <= 109*/

//解题思路：
//使用哈希表查重
//处理哈希冲突：当哈希索引位置已经被其他元素占据时，通过线性探测（逐步增加索引）找到下一个空闲位置。


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

bool containsDuplicate(const int* nums, int numsSize) {
    printf("开始检查数组是否包含重复元素。\n");

    // 创建哈希表，用于存储已经出现过的数字
    int* hashTable = (int*)calloc(2 * numsSize, sizeof(int));
    if (!hashTable) {
        printf("内存分配失败。\n");
        return false;
    }

    for (int i = 0; i < numsSize; ++i) {
        int hashIndex = abs(nums[i] % numsSize);
        printf("处理元素: %d，哈希索引: %d\n", nums[i], hashIndex);

        // 线性探测解决冲突
        while (hashTable[hashIndex] != 0) {
            printf("哈希索引 %d 已被占用，值为 %d。\n", hashIndex, hashTable[hashIndex]);
            if (hashTable[hashIndex] == nums[i]) {
                printf("找到重复元素: %d\n", nums[i]);
                free(hashTable);
                return true;
            }
            hashIndex = (hashIndex + 1) % numsSize;
        }

        printf("将元素 %d 存储在哈希表索引 %d。\n", nums[i], hashIndex);
        hashTable[hashIndex] = nums[i];
    }

    printf("未找到任何重复元素。\n");
    free(hashTable);
    return false;
}

int main() {
    int nums1[] = {1, 2, 3, 1};
    int nums2[] = {1, 2, 3, 4};
    int nums3[] = {1, 1, 1, 3, 3, 4, 3, 2, 4, 2};

    printf("示例 1:\n");
    printf("输出：%s\n\n", containsDuplicate(nums1, 4) ? "true" : "false");

    printf("示例 2:\n");
    printf("输出：%s\n\n", containsDuplicate(nums2, 4) ? "true" : "false");

    printf("示例 3:\n");
    printf("输出：%s\n\n", containsDuplicate(nums3, 10) ? "true" : "false");

    return 0;
}
