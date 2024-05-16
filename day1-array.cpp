#include <iostream>
#include <vector>

int removeDuplicates(std::vector<int>& nums) {
    if (nums.empty()) return 0; // 空数组直接返回 0

    int k = 1; // 记录唯一元素的数量，初始值为 1（因为第一个元素必然是唯一的）
    for (int i = 1; i < nums.size(); ++i) {
        if (nums[i] != nums[i - 1]) { // 如果当前元素与前一个元素不相同
            nums[k++] = nums[i]; // 将当前元素移到数组的正确位置
        }
    }
    return k; // 返回唯一元素的数量
}

int main() {
    std::vector<int> nums = {1, 1, 2, 2, 3, 4, 5, 5, 5};
    int k = removeDuplicates(nums);
    std::cout << "Number of unique elements: " << k << std::endl;
    std::cout << "Updated array: ";
    for (int i = 0; i < k; ++i) {
        std::cout << nums[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}
