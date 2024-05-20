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
