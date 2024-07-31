#include "task_scheduler.h"
#include <stdbool.h>

typedef struct {
    int processingTime;
    int index;
} HeapNode;

typedef struct {
    HeapNode* nodes;
    int size;
    int capacity;
} MinHeap;

static void swap(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

static void heapifyDown(MinHeap* heap, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < heap->size && (heap->nodes[left].processingTime < heap->nodes[smallest].processingTime ||
        (heap->nodes[left].processingTime == heap->nodes[smallest].processingTime && heap->nodes[left].index < heap->nodes[smallest].index))) {
        smallest = left;
    }

    if (right < heap->size && (heap->nodes[right].processingTime < heap->nodes[smallest].processingTime ||
        (heap->nodes[right].processingTime == heap->nodes[smallest].processingTime && heap->nodes[right].index < heap->nodes[smallest].index))) {
        smallest = right;
    }

    if (smallest != index) {
        swap(&heap->nodes[smallest], &heap->nodes[index]);
        heapifyDown(heap, smallest);
    }
}

static void heapifyUp(MinHeap* heap, int index) {
    if (index && (heap->nodes[index].processingTime < heap->nodes[(index - 1) / 2].processingTime ||
        (heap->nodes[index].processingTime == heap->nodes[(index - 1) / 2].processingTime && heap->nodes[index].index < heap->nodes[(index - 1) / 2].index))) {
        swap(&heap->nodes[index], &heap->nodes[(index - 1) / 2]);
        heapifyUp(heap, (index - 1) / 2);
    }
}

static void push(MinHeap* heap, int processingTime, int index) {
    if (heap->size == heap->capacity) {
        heap->capacity *= 2;
        heap->nodes = realloc(heap->nodes, heap->capacity * sizeof(HeapNode));
    }
    heap->nodes[heap->size++] = (HeapNode){processingTime, index};
    heapifyUp(heap, heap->size - 1);
}

static void pop(MinHeap* heap) {
    heap->nodes[0] = heap->nodes[--heap->size];
    heapifyDown(heap, 0);
}

static HeapNode top(const MinHeap* heap) {
    return heap->nodes[0];
}

static bool isEmpty(const MinHeap* heap) {
    return heap->size == 0;
}

static int compareTasks(const void* a, const void* b) {
    return ((Task*)a)->enqueueTime - ((Task*)b)->enqueueTime;
}

void getOrder(const Task* tasks, const int tasksSize, int* result, int* resultSize) {
    Task* sortedTasks = malloc(tasksSize * sizeof(Task));
    for (int i = 0; i < tasksSize; ++i) {
        sortedTasks[i] = tasks[i];
    }
    
    qsort(sortedTasks, tasksSize, sizeof(Task), compareTasks);

    MinHeap heap = {malloc(tasksSize * sizeof(HeapNode)), 0, tasksSize};
    int currentTime = 0, taskIndex = 0, resultIndex = 0;

    while (taskIndex < tasksSize || !isEmpty(&heap)) {
        if (isEmpty(&heap)) {
            currentTime = sortedTasks[taskIndex].enqueueTime;
        }
        
        while (taskIndex < tasksSize && sortedTasks[taskIndex].enqueueTime <= currentTime) {
            printf("Adding task %d to the heap\n", sortedTasks[taskIndex].index);
            push(&heap, sortedTasks[taskIndex].processingTime, sortedTasks[taskIndex].index);
            ++taskIndex;
        }

        HeapNode currentTask = top(&heap);
        pop(&heap);
        printf("Executing task %d at time %d\n", currentTask.index, currentTime);
        result[resultIndex++] = currentTask.index;
        currentTime += currentTask.processingTime;
    }
    
    free(heap.nodes);
    free(sortedTasks);
    *resultSize = resultIndex;
}
