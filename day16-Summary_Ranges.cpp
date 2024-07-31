/*给定一个  无重复元素 的 有序 整数数组 nums 。

返回 恰好覆盖数组中所有数字 的 最小有序 区间范围列表 。也就是说，nums 的每个元素都恰好被某个区间范围所覆盖，并且不存在属于某个范围但不属于 nums 的数字 x 。

列表中的每个区间范围 [a,b] 应该按如下格式输出：

"a->b" ，如果 a != b
"a" ，如果 a == b
 

示例 1：

输入：nums = [0,1,2,4,5,7]
输出：["0->2","4->5","7"]
解释：区间范围是：
[0,2] --> "0->2"
[4,5] --> "4->5"
[7,7] --> "7" */

#include <iostream>
#include <vector>
#include <string>

std::vector<std::string> summaryRanges(const std::vector<int>& nums) {
    std::vector<std::string> ranges;
    int n = nums.size();
    
    if (n == 0) return ranges;
    
    int start = nums[0];
    
    for (int i = 1; i <= n; ++i) {
        printf("Current index: %d\n", i);
        if (i == n || nums[i] != nums[i - 1] + 1) {
            if (nums[i - 1] == start) {
                ranges.push_back(std::to_string(start));
                printf("Adding range: %s\n", std::to_string(start).c_str());
            } else {
                ranges.push_back(std::to_string(start) + "->" + std::to_string(nums[i - 1]));
                printf("Adding range: %s\n", (std::to_string(start) + "->" + std::to_string(nums[i - 1])).c_str());
            }
            if (i < n) {
                start = nums[i];
                printf("New start: %d\n", start);
            }
        }
    }
    
    return ranges;
}

int main() {
    const std::vector<int> nums = {0, 1, 2, 4, 5, 7};
    std::vector<std::string> result = summaryRanges(nums);
    
    printf("Final result:\n");
    for (const std::string& range : result) {
        printf("%s\n", range.c_str());
    }
    
    return 0;
}
