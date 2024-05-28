/*给定一个数组 nums，编写一个函数将所有 0 移动到数组的末尾，同时保持非零元素的相对顺序。

请注意 ，必须在不复制数组的情况下原地对数组进行操作。
 

示例 1:

输入: nums = [0,1,0,3,12]
输出: [1,3,12,0,0]
示例 2:

输入: nums = [0]
输出: [0]
 

提示:

1 <= nums.length <= 104
-231 <= nums[i] <= 231 - 1

进阶：你能尽量减少完成的操作次数吗？*/

//解题思路：
//不复制数组的情况下原地进行操作，通过遍历数组，将非零元素移动到数组前面，剩余位置补令

#include <stdio.h>

void moveZeroes(int* nums, const int numsSize) {
    int nonZeroIndex = 0; // 用于记录非零元素的位置

    // 将非零元素移动到数组的前部分
    for (int i = 0; i < numsSize; i++) {
        if (nums[i] != 0) {
            printf("遇到非零元素 nums[%d] = %d，将其移动到 nums[%d]\n", i, nums[i], nonZeroIndex);
            nums[nonZeroIndex] = nums[i]; // 将非零元素移到 nonZeroIndex 所指向的位置
            nonZeroIndex++; // 非零元素位置索引自增
        }
        else {
            printf("遇到零元素 nums[%d] = 0，跳过不处理\n", i);
        }
    }

    printf("\n");

    // 将剩余位置补零
    for (int i = nonZeroIndex; i < numsSize; i++) {
        printf("将 nums[%d] 的值置零\n", i);
        nums[i] = 0; // 将剩余位置的值置零
    }
}


int main() {
    int nums1[] = {0, 1, 0, 3, 12};
    int nums2[] = {0};
    int nums3[] = {1, 2, 3, 4, 0, 5, 6, 0};

    const int size1 = sizeof(nums1) / sizeof(nums1[0]);
    moveZeroes(nums1, size1);
    printf("结果为：");
    for (int i = 0; i < size1; i++) {
        printf("%d ", nums1[i]);
    }
    printf("\n\n");

    const int size2 = sizeof(nums2) / sizeof(nums2[0]);
    moveZeroes(nums2, size2);
    printf("结果为：");
    for (int i = 0; i < size2; i++) {
        printf("%d ", nums2[i]);
    }
    printf("\n\n");

    const int size3 = sizeof(nums3) / sizeof(nums3[0]);
    moveZeroes(nums3, size3);
    printf("结果为：");
    for (int i = 0; i < size3; i++) {
        printf("%d ", nums3[i]);
    }
    printf("\n\n");

    return 0;
}
