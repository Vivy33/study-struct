/*
你的朋友正在使用键盘输入他的名字 name。偶尔，在键入字符 c 时，按键可能会被长按，而字符可能被输入 1 次或多次。

你将会检查键盘输入的字符 typed。如果它对应的可能是你的朋友的名字（其中一些字符可能被长按），那么就返回 True。

 
示例 1：

输入：name = "alex", typed = "aaleex"
输出：true
解释：'alex' 中的 'a' 和 'e' 被长按。
示例 2：

输入：name = "saeed", typed = "ssaaedd"
输出：false
解释：'e' 一定需要被键入两次，但在 typed 的输出中不是这样。
 

提示：

1 <= name.length, typed.length <= 1000
name 和 typed 的字符都是小写字母
*/

//解题思路：利用双指针同步前进
//如果当前 name[i] 与 typed[j] 相同，则两个指针同时前进，即 i++ 和 j++。
//如果 name[i] 与 typed[j] 不同，但 typed[j] 与 typed[j-1] 相同，说明 typed[j] 是由于长按造成的重复字符，此时只需前进 j，即 j++。
//如果上述两个条件都不满足，说明字符不匹配，直接返回 false。

//优点：
//双指针同步前进在一个遍历过程中完成匹配检查，时间复杂度为 O(n)

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

bool isLongPressedName(const char *name, const char *typed) {
    int i = 0, j = 0;
    const int nameLength = strlen(name);
    const int typedLength = strlen(typed);

    while (j < typedLength) {
        printf("检查 typed[%d] = %c 与 name[%d] = %c\n", j, typed[j], i, (i < nameLength) ? name[i] : '-');
        if (i < nameLength && name[i] == typed[j]) {
            i++;
        } else if (j == 0 || typed[j] != typed[j - 1]) {
            printf("在 typed[%d] = %c 处发现不匹配\n", j, typed[j]);
            return false;
        }
        j++;
    }

    printf("已到达 typed 输入末尾。i = %d, nameLength = %d\n", i, nameLength);
    return i == nameLength;
}

int main() {
    const char *name1 = "alex";
    const char *typed1 = "aaleex";
    const char *name2 = "saeed";
    const char *typed2 = "ssaaedd";

    printf("测试用例 1: name = %s, typed = %s\n", name1, typed1);
    printf("结果: %s\n", isLongPressedName(name1, typed1) ? "是" : "否");
    printf("\n");

    printf("测试用例 2: name = %s, typed = %s\n", name2, typed2);
    printf("结果: %s\n", isLongPressedName(name2, typed2) ? "是" : "否");
    printf("\n");

    return 0;
}
