/*给定一个排序数组和一个目标值，在数组中找到目标值，并返回其索引。如果目标值不存在于数组中，返回它将会被按顺序插入的位置。

请必须使用时间复杂度为 O(log n) 的算法。

 

示例 1:

输入: nums = [1,3,5,6], target = 5
输出: 2
示例 2:

输入: nums = [1,3,5,6], target = 2
输出: 1
示例 3:

输入: nums = [1,3,5,6], target = 7
输出: 4
 

提示:

1 <= nums.length <= 104
-104 <= nums[i] <= 104
nums 为 无重复元素 的 升序 排列数组
-104 <= target <= 104*/

#include <stdio.h>

// 二分查找函数，返回目标值在数组中的索引，如果目标值不存在，则返回应该插入的位置
int searchInsert(const int* nums, const int numsSize, const int target) {
    int left = 0, right = numsSize - 1;
    while (left <= right) {
        printf("循环开始 -> 当前left: %d, right: %d\n", left, right);
        const int mid = left + (right - left) / 2;
        printf("  计算mid -> left: %d, right: %d, mid: %d\n", left, right, mid);

        if (nums[mid] == target) {
            printf("找到目标值 -> mid: %d\n", mid);
            return mid;
        } else if (nums[mid] < target) {
            printf("    目标值大于中间值 -> nums[mid]: %d, target: %d -> 更新left: %d -> %d\n", nums[mid], target, left, mid + 1);
            left = mid + 1;
        } else {
            printf("    目标值小于中间值 -> nums[mid]: %d, target: %d -> 更新right: %d -> %d\n", nums[mid], target, right, mid - 1);
            right = mid - 1;
        }
    }
    printf("退出循环，left即为插入位置 -> left: %d\n", left);
    return left; // 当退出循环时，left 即为应该插入的位置
}

int main() {
    // 测试样例
    const int nums1[] = {1, 3, 5, 6};
    const int target1 = 5;
    const int result1 = searchInsert(nums1, 4, target1);
    printf("输入: nums = [1,3,5,6], target = %d\n输出: %d\n", target1, result1);
    printf("\n");

    const int nums2[] = {1, 3, 5, 6};
    const int target2 = 2;
    const int result2 = searchInsert(nums2, 4, target2);
    printf("输入: nums = [1,3,5,6], target = %d\n输出: %d\n", target2, result2);
    printf("\n");

    const int nums3[] = {1, 3, 5, 6};
    const int target3 = 7;
    const int result3 = searchInsert(nums3, 4, target3);
    printf("输入: nums = [1,3,5,6], target = %d\n输出: %d\n", target3, result3);
    printf("\n");

    return 0;
}
