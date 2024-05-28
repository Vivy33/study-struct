/*编写一个函数，其作用是将输入的字符串反转过来。输入字符串以字符数组 s 的形式给出。

不要给另外的数组分配额外的空间，你必须原地修改输入数组、使用 O(1) 的额外空间解决这一问题。

 
示例 1：

输入：s = ["h","e","l","l","o"]
输出：["o","l","l","e","h"]
示例 2：

输入：s = ["H","a","n","n","a","h"]
输出：["h","a","n","n","a","H"]
 

提示：

1 <= s.length <= 105
s[i] 都是 ASCII 码表中的可打印字符*/

#include <stdio.h>

void reverseString(char* s, const int sSize) {
    // 定义左右指针，分别指向字符串的起始和末尾位置
    int left = 0, right = sSize - 1;

    // 逐步交换左右指针所指向的字符，直到左右指针相遇
    while (left < right) {
        // 输出当前操作
        printf("交换字符：%c 和 %c\n", s[left], s[right]);

        // 交换左右指针所指向的字符
        char temp = s[left];
        s[left] = s[right];
        s[right] = temp;

        // 移动左右指针
        left++;
        right--;
    }
}

int main() {
    char s1[] = {'h', 'e', 'l', 'l', 'o'};
    char s2[] = {'H', 'a', 'n', 'n', 'a', 'h'};

    const int size1 = sizeof(s1) / sizeof(s1[0]);
    reverseString(s1, size1);
    printf("结果为：");
    for (int i = 0; i < size1; i++) {
        printf("%c ", s1[i]);
    }
    printf("\n");

    const int size2 = sizeof(s2) / sizeof(s2[0]);
    reverseString(s2, size2);
    printf("结果为：");
    for (int i = 0; i < size2; i++) {
        printf("%c ", s2[i]);
    }
    printf("\n");

    return 0;
}
