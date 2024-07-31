#include "task_scheduler.h"

int main() {
    const Task tasks[] = {
        {1, 2, 0},
        {2, 4, 1},
        {3, 2, 2},
        {4, 1, 3}
    };
    const int tasksSize = sizeof(tasks) / sizeof(tasks[0]);
    int result[tasksSize];
    int resultSize;

    getOrder(tasks, tasksSize, result, &resultSize);

    printf("Task execution order:\n");
    for (int i = 0; i < resultSize; ++i) {
        printf("%d ", result[i]);
    }
    printf("\n");

    return 0;
}
