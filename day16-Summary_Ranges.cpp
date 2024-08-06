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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int start;
    int end;
} Range;

Range* summaryRanges(const int* nums, int numsSize, int* returnSize) {
    if (numsSize == 0) {
        *returnSize = 0;
        return NULL;
    }

    Range* ranges = (Range*)malloc(numsSize * sizeof(Range));
    int rangeCount = 0;

    int start = nums[0];

    for (int i = 1; i <= numsSize; ++i) {
        if (i == numsSize || nums[i] != nums[i - 1] + 1) {
            ranges[rangeCount].start = start;
            ranges[rangeCount].end = nums[i - 1];
            rangeCount++;
            if (i < numsSize) {
                start = nums[i];
            }
        }
    }

    *returnSize = rangeCount;
    return ranges;
}

void printRanges(const Range* ranges, int size) {
    for (int i = 0; i < size; ++i) {
        if (ranges[i].start == ranges[i].end) {
            printf("%d\n", ranges[i].start);
        } else {
            printf("%d->%d\n", ranges[i].start, ranges[i].end);
        }
    }
}

int main() {
    const int nums[] = {0, 1, 2, 4, 5, 7};
    int numsSize = sizeof(nums) / sizeof(nums[0]);
    int returnSize;

    Range* result = summaryRanges(nums, numsSize, &returnSize);

    printf("Final result:\n");
    printRanges(result, returnSize);

    free(result);
    return 0;
}
