#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>

#include "workqueue.c"

typedef struct SharedData {
    int *counter;
    HANDLE mutex;
} SharedData;

void *IncrementCounterSafeTask(void *data)
{
    SharedData *sharedData = (SharedData*)data;
    
    // waiting until mutex is released from other thread
    WaitForSingleObject(sharedData->mutex, INFINITE);

    int *counter = sharedData->counter;
    (*counter) += 1;

    ReleaseMutex(sharedData->mutex);

    return 0;
}

void *IncrementCounterTask(void *data)
{
    int *counter = (int*)data;
    (*counter) += 1;
    return 0;
}

#define THREAD_COUNT 8

int main(int argc, char **argv)
{ 
    WorkQueue *workQueue = CreateWorkQueueAndWorkerThreads(THREAD_COUNT);

    int countUpto = 100000;

    printf("Counting upto: %d\n", countUpto);

    int sharedCounter = 0;

    for(int n = 0; n < countUpto; n++) 
    {
        WorkQueueEntry entry = {0};
        entry.callback = IncrementCounterTask;
        entry.data = (void*)&sharedCounter;
        entry.isCompleted = FALSE;
        PushWork(workQueue, entry);
    }

    WaitUntilCompletion(workQueue);

    printf("Without Mutex Counter: %d\n", sharedCounter);

    workQueue->entryCount = 0;
    workQueue->nextEntryTodo = 0;

    sharedCounter = 0;

    HANDLE mutex = CreateMutex(0, FALSE, 0);

    SharedData sharedData = {0};
    sharedData.counter = &sharedCounter;
    sharedData.mutex = mutex;

    for(int n = 0; n < countUpto; n++) 
    {
        WorkQueueEntry entry = {0};
        entry.callback = IncrementCounterSafeTask;
        entry.data = (void*)&sharedData;
        entry.isCompleted = FALSE;
        PushWork(workQueue, entry);
    }

    WaitUntilCompletion(workQueue);

    printf("With Mutex Counter: %d\n", sharedCounter);
    
    return 0;
}
