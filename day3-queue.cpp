/*请你仅使用两个队列实现一个后入先出（LIFO）的栈，并支持普通栈的全部四种操作（push、top、pop 和 empty）。

实现 MyStack 类：

void push(int x) 将元素 x 压入栈顶。
int pop() 移除并返回栈顶元素。
int top() 返回栈顶元素。
boolean empty() 如果栈是空的，返回 true ；否则，返回 false 。*/

#include <iostream>
#include <queue>

using namespace std;

class MyStack {
private:
    queue<int> q1; // 主队列，用于存储栈的元素
    queue<int> q2; // 辅助队列，用于辅助 pop 操作
    int top_element; // 记录栈顶元素

public:
    MyStack() {
    }
    
    //将元素 x 推到栈堆上
    void push(int x) {
        q1.push(x);
        top_element = x;
        printf("将元素 %d 推到堆栈上\n", x);
    }
    
    //删除堆栈顶部的元素并返回该元素
    int pop() {
        // 将 q1 中的元素转移到 q2 中，直到 q1 中只剩下一个元素
        while (q1.size() > 1) {
            printf("将元素 %d 从q1移动到q2\n", q1.front());
            top_element = q1.front();
            q2.push(top_element);
            q1.pop();
            printf("\n");
        }
        // 移除并返回栈顶元素
        int result = q1.front();
        q1.pop();
        printf("pop出元素 %d 到堆栈上\n", result);
        // 交换 q1 和 q2，以确保 q1 中存储的始终是栈的元素
        swap(q1, q2);
        return result;
    }
    
    int top() {
        // 返回栈顶元素
        return top_element;
    }

    bool empty() {
        // 检查 q1 是否为空，若为空则栈为空
        return q1.empty();
    }
};

int main() {
    MyStack myStack;
    myStack.push(1);
    myStack.push(2);
    printf("返回: %d\n", myStack.top());// 返回 2
    printf("返回: %d\n", myStack.pop()); // 返回 2
    printf("返回: %d\n", myStack.empty()); // 返回 False
    
    return 0;
}
