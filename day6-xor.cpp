/*给你一个 非空 整数数组 nums ，除了某个元素只出现一次以外，其余每个元素均出现两次。找出那个只出现了一次的元素。

你必须设计并实现线性时间复杂度的算法来解决此问题，且该算法只使用常量额外空间。

示例 1 ：

输入：nums = [2,2,1]
输出：1
示例 2 ：

输入：nums = [4,1,2,1,2]
输出：4
示例 3 ：

输入：nums = [1]
输出：1

提示：

1 <= nums.length <= 3 * 104
-3 * 104 <= nums[i] <= 3 * 104
除了某个元素只出现一次以外，其余每个元素均出现两次。*/

//解题思路：利用异或运算的性质来解决 
//异或运算有以下性质：
//任何数和 0 做异或运算，结果仍然是原来的数：a ⊕ 0 = a
//任何数和其自身做异或运算，结果是 0：a ⊕ a = 0
//异或运算满足交换律和结合律：a ⊕ b ⊕ a = (a ⊕ a) ⊕ b = 0 ⊕ b = b
//由于除了一个元素只出现一次之外，其他所有元素都出现两次，那么将所有元素进行异或运算，相同的元素会相互抵消，最终留下的就是只出现一次的元素。

#include <stdio.h>

#define Len(a) (sizeof(a) / sizeof(a[0]))

int singleNumber(const int* nums, const int numsSize) {
    int result = 0;
    for (int i = 0; i < numsSize; i++) {
        result ^= nums[i];
        printf("当前操作：nums[%d] ^= %d\n", i, nums[i]);
    }
    return result;
}

void printResult(const int* nums, const int numsSize) {
    int result = singleNumber(nums, numsSize);
    printf("结果为：%d\n\n", result);
}

int main() {
    const int nums1[] = {2, 2, 1};
    const int nums2[] = {4, 1, 2, 1, 2};
    const int nums3[] = {1};

    printResult(nums1, Len(nums1));
    printResult(nums2, Len(nums2));
    printResult(nums3, Len(nums3));

    return 0;
}
