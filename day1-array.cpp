1.删除有序数组中的重复项

给你一个非严格递增排列的数组 nums ，请你原地删除重复出现的元素，使每个元素只出现一次，返回删除后数组的新长度。元素的相对顺序应该保持一致 。然后返回 nums 中唯一元素的个数。

考虑 nums 的唯一元素的数量为 j ，你需要做以下事情确保你的题解可以被通过：

更改数组 nums ，使 nums 的前 j 个元素包含唯一元素，并按照它们最初在 nums 中出现的顺序排列。nums 的其余元素与 nums 的大小不重要。
返回 j 。

#include <iostream>
#include <vector>

int removeDuplicates(std::vector<int>& nums) {
    if (nums.empty()) return 0; // 空数组直接返回 0

    int j = 1; // 记录唯一元素的数量，初始值为 1（因为第一个元素必然是唯一的）
    for (int i = 1; i < nums.size(); ++i) {
        if (nums[i] != nums[i - 1]) { // 如果当前元素与前一个元素不相同
            nums[j++] = nums[i]; // 将当前元素移到数组的正确位置
        }
    }
    return j; // 返回唯一元素的数量
}

int main() {
    std::vector<int> nums = {1, 1, 2, 2, 3, 4, 5, 5, 5};
    int j = removeDuplicates(nums);
    std::cout << "Number of unique elements: " << j << std::endl;
    std::cout << "Updated array: ";
    for (int i = 0; i < j; ++i) {
        std::cout << nums[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}
