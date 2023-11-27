#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>

#include "workqueue.c"

static HANDLE mutex = 0;

void *IncrementCounterSafeTask(void *data)
{
    // waiting until mutex is released from other thread
    WaitForSingleObject(mutex, INFINITE);

    int *counter = (int*)data;
    (*counter) += 1;

    ReleaseMutex(mutex);

    return 0;
}

void *IncrementCounterTask(void *data)
{
    int *counter = (int*)data;
    (*counter) += 1;
    return 0;
}

#define THREAD_COUNT 6

static WorkQueue workQueue = {0};

int main(int argc, char **argv)
{ 
    HANDLE semaphore = CreateSemaphore(0, 0, THREAD_COUNT, 0);

    workQueue.semaphore = semaphore;

    SIZE_T stackSize = 0;

    ThreadContext contexts[THREAD_COUNT] = {0};

    for(int n = 0; n < THREAD_COUNT; n++)
    {
        ThreadContext *context = &contexts[n];
        context->threadId = n;
        context->queue = &workQueue;
        context->workDoneCount = 0;
        HANDLE threadHandle = CreateThread(0, stackSize, DoWorkFromQueue, context, 0, 0);
    }

    int countUpto = 100000;

    printf("Counting upto: %d\n", countUpto);

    int sharedCounter = 0;

    for(int n = 0; n < countUpto; n++) 
    {
        WorkQueueEntry entry = {0};
        entry.callback = IncrementCounterTask;
        entry.data = (void*)&sharedCounter;
        entry.isCompleted = FALSE;
        PushWork(&workQueue, entry);
    }

    while(TRUE)
    {
        boolean allCompleted = TRUE;
        for(int n = 0; n < workQueue.entryCount; n++)
        {
            if(!workQueue.entries[n].isCompleted)
            {
                allCompleted = FALSE;
                break;
            }
        }
        if(allCompleted) break;
    }

    printf("Without Mutex Counter: %d\n", sharedCounter);

    workQueue.entryCount = 0;
    workQueue.nextEntryTodo = 0;
    sharedCounter = 0;

    mutex = CreateMutex(0, FALSE, 0);

    for(int n = 0; n < countUpto; n++) 
    {
        WorkQueueEntry entry = {0};
        entry.callback = IncrementCounterSafeTask;
        entry.data = (void*)&sharedCounter;
        entry.isCompleted = FALSE;
        PushWork(&workQueue, entry);
    }

    while(TRUE)
    {
        boolean allCompleted = TRUE;
        for(int n = 0; n < workQueue.entryCount; n++)
        {
            if(!workQueue.entries[n].isCompleted)
            {
                allCompleted = FALSE;
                break;
            }
        }
        if(allCompleted) break;
    }

    printf("With Mutex Counter: %d\n", sharedCounter);
    
    return 0;
}
