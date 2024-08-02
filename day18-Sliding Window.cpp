/*给定一个字符串 s ，请你找出其中不含有重复字符的 最长子串的长度。

示例 1:

输入: s = "abcabcbb"
输出: 3 
解释: 因为无重复字符的最长子串是 "abc"，所以其长度为 3。

提示：
s 由英文字母、数字、符号和空格组成*/

#include <stdio.h>
#include <string.h>

int lengthOfLongestSubstring(const char* s) {
    const int n = strlen(s);
    int maxLength = 0;
    int start = 0;
    int index[128] = {-1};  // 记录每个字符上次出现的位置，ASCII字符范围是0-127

    for (int i = 0; i < 128; ++i) {
        index[i] = -1;
    }

    for (int end = 0; end < n; ++end) {
        const char currentChar = s[end];
        printf("Checking character s[%d] = '%c'\n", end, currentChar);

        if (index[currentChar] >= start) {
            start = index[currentChar] + 1;
            printf("Duplicate found. Move start to %d\n", start);
        }

        index[currentChar] = end;
        printf("Update index of '%c' to %d\n", currentChar, end);

        const int currentLength = end - start + 1;
        if (currentLength > maxLength) {
            maxLength = currentLength;
            printf("Update maxLength to %d\n", maxLength);
        }
    }

    return maxLength;
}

int main() {
    const char* s = "abcabcbb";
    int length = lengthOfLongestSubstring(s);
    printf("The length of the longest substring without repeating characters is %d\n", length);
    return 0;
}
