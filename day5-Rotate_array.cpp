/*给定一个整数数组 nums，将数组中的元素向右轮转 k 个位置，其中 k 是非负数。

示例 1:

输入: nums = [1,2,3,4,5,6,7], k = 3
输出: [5,6,7,1,2,3,4]
解释:
向右轮转 1 步: [7,1,2,3,4,5,6]
向右轮转 2 步: [6,7,1,2,3,4,5]
向右轮转 3 步: [5,6,7,1,2,3,4]
示例 2:

输入：nums = [-1,-100,3,99], k = 2
输出：[3,99,-1,-100]
解释: 
向右轮转 1 步: [99,-1,-100,3]
向右轮转 2 步: [3,99,-1,-100]

提示：

1 <= nums.length <= 105
-231 <= nums[i] <= 231 - 1
0 <= k <= 105

进阶：

尽可能想出更多的解决方案，至少有 三种 不同的方法可以解决这个问题。
你可以使用空间复杂度为 O(1) 的 原地 算法解决这个问题吗？*/

//##方法一：使用额外的数组
#include <stdio.h>
#include <stdlib.h>

void rotate(const int* nums, int numsSize, int k) {
    k = k % numsSize;//保证k不超过数组长度
    int* rotated = (int*)malloc(numsSize* sizeof(int));
    if (rotated == NULL) {
        return;//处理内存分配失败
    }

//将数组进行 右轮转存 并存储在 临时数组 中
        for (int i = 0; i < numsSize; ++i) {
        rotated[(i + k) % numsSize] = nums[i];
        printf("rotated[%d] = nums[%d] -> rotated[%d] = %d\n", (i + k) % numsSize, i, (i + k) % numsSize, nums[i]);
        }

        printf("\n");

//将 临时数组 中的结果复制回 原数组
        for (int i = 0; i < numsSize; ++i) {
        printf("nums[%d] = rotated[%d] -> nums[%d] = %d\n", i, i, i, rotated[i]);
        ((int*)nums)[i] = rotated[i];//const修饰过的指针需要通过 类型转换 解除常量性
        }

    printf("\n");
    free(rotated);
}

int main() {
    int nums[] = {1, 2, 3, 4, 5, 6, 7};
    const int numsSize = sizeof(nums) / sizeof(nums[0]);
    const int k = 3;
    rotate(nums, numsSize, k);
    for (int i = 0; i < numsSize; ++i) {
        printf("%d", nums[i]);
    }
    printf("\n");
    return 0;
}

//方法二：

