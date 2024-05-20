/*请你仅使用两个队列实现一个后入先出（LIFO）的栈，并支持普通栈的全部四种操作（push、top、pop 和 empty）。

实现 MyStack 类：

void push(int x) 将元素 x 压入栈顶。
int pop() 移除并返回栈顶元素。
int top() 返回栈顶元素。
boolean empty() 如果栈是空的，返回 true ；否则，返回 false 。
 

注意：

你只能使用队列的标准操作 —— 也就是 push to back、peek/pop from front、size 和 is empty 这些操作。
你所使用的语言也许不支持队列。 你可以使用 list （列表）或者 deque（双端队列）来模拟一个队列 , 只要是标准的队列操作即可。
 

示例：

输入：
["MyStack", "push", "push", "top", "pop", "empty"]
[[], [1], [2], [], [], []]
输出：
[null, null, null, 2, 2, false]

解释：
MyStack myStack = new MyStack();
myStack.push(1);
myStack.push(2);
myStack.top(); // 返回 2
myStack.pop(); // 返回 2
myStack.empty(); // 返回 False
 

提示：

1 <= x <= 9
最多调用100 次 push、pop、top 和 empty
每次调用 pop 和 top 都保证栈不为空*/

#include <stdio.h>
#include <stdlib.h>

// 定义栈结构
struct MyStack {
    int *array; // 用于存储栈的元素
    int top_element; // 记录栈顶元素的索引
    int capacity; // 栈的容量
};

// 初始化栈
struct MyStack* createStack(int capacity) {
    struct MyStack* stack = (struct MyStack*)malloc(sizeof(struct MyStack));
    stack->capacity = capacity;
    stack->top_element = -1;
    stack->array = (int*)malloc(stack->capacity * sizeof(int));
    return stack;
}

// 判断栈是否已满
int isFull(struct MyStack* stack) {
    return stack->top_element == stack->capacity - 1;
}

// 判断栈是否为空
int isEmpty(struct MyStack* stack) {
    return stack->top_element == -1;
}

// 将元素压入栈顶
void push(struct MyStack* stack, int item) {
    if (isFull(stack)) {
        printf("Stack is full, cannot push element %d\n", item);
        return;
    }
    stack->array[++stack->top_element] = item;
    printf("Pushing element %d onto the stack.\n", item);
}

// 删除并返回栈顶元素
int pop(struct MyStack* stack) {
    if (isEmpty(stack)) {
        printf("Stack is empty, cannot pop.\n");
        return -1;
    }
    int popped_element = stack->array[stack->top_element--];
    printf("Popping element %d from the stack.\n", popped_element);
    return popped_element;
}

// 返回栈顶元素
int top(struct MyStack* stack) {
    if (isEmpty(stack)) {
        printf("Stack is empty, no top element.\n");
        return -1;
    }
    return stack->array[stack->top_element];
}

// 主函数
int main() {
    struct MyStack* myStack = createStack(100);
    push(myStack, 1);
    push(myStack, 2);
    printf("Top element: %d\n", top(myStack)); // 返回 2
    printf("Popped element: %d\n", pop(myStack)); // 返回 2
    printf("Is the stack empty? %s\n", isEmpty(myStack) ? "true" : "false"); // 返回 False
    
    return 0;
}
