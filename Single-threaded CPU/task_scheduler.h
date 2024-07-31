#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int enqueueTime;
    int processingTime;
    int index;
} Task;

void getOrder(const Task* tasks, int tasksSize, int* result, int* resultSize);

#endif
