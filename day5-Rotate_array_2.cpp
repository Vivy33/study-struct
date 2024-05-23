//方法二：循环替换 （原地算法）

#include <stdio.h>

void rotate(int* nums, int numsSize, int k) {
    k = k % numsSize; // 确保 k 不超过数组长度
    int count = 0; // 记录已经置换位置的元素数量

    for (int start = 0; count < numsSize; ++start) {
        int current = start;
        int prev = nums[start];

        printf("开始新的循环周期，起始索引 %d\n", start);

        do {
            int next = (current + k) % numsSize;
            int temp = nums[next];
            nums[next] = prev;
            printf("nums[%d] = %d -> nums[%d] = %d\n", next, temp, next, prev);
            prev = temp;
            current = next;
            count++;
        } while (start != current);
        printf("循环周期结束，起始索引 %d\n\n", start);
    }
}

int main() {
    int nums[] = {1, 2, 3, 4, 5, 6, 7};
    const int numsSize = sizeof(nums) / sizeof(nums[0]);
    const int k = 3;
    rotate(nums, numsSize, k);
    for (int i = 0; i < numsSize; ++i) {
        printf("%d ", nums[i]);
    }
    printf("\n");
    return 0;
}
