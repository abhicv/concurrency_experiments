#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>

typedef void* (*CallBack)(void *data);
typedef struct WorkQueueEntry
{
    CallBack callback;
    void *data;
    void *result;
    bool isCompleted;
} WorkQueueEntry;

#define MAX_QUEUE_SIZE 1000000

typedef struct WorkQueue
{
    WorkQueueEntry entries[MAX_QUEUE_SIZE];  
    // WorkQueueEntry *entries;
    volatile size_t entryCount;
    volatile size_t nextEntryTodo;
    HANDLE semaphore;
} WorkQueue;

typedef struct ThreadContext
{
    unsigned int threadId;
    unsigned int workDoneCount;
    WorkQueue *queue;
} ThreadContext;


void PushWork(WorkQueue *queue, WorkQueueEntry entry)
{
    if(queue->entryCount >= MAX_QUEUE_SIZE)
    {
        printf("warning: queue is full!\n");
        return;
    }

    queue->entries[queue->entryCount] = entry;
    MemoryBarrier();
    queue->entryCount++;
    ReleaseSemaphore(queue->semaphore, 1, 0);
}

DWORD DoWorkFromQueue(LPVOID lpParameter)
{
    ThreadContext *context = (ThreadContext*)lpParameter;
    // printf("Thread %ld spawned\n", context->threadId);

    WorkQueue *queue = context->queue;
    while(1)
    {
        unsigned int entryTodo = queue->nextEntryTodo;
        if(entryTodo < queue->entryCount)
        {
            unsigned int index = InterlockedCompareExchange((long volatile*)&queue->nextEntryTodo, entryTodo + 1, entryTodo);
            if(index == entryTodo)
            {
                // printf("Work %d done on thread %ld\n", entryTodo, context->threadId);
                WorkQueueEntry *entry = &queue->entries[entryTodo];
                entry->result = entry->callback(entry->data);
                entry->isCompleted = true;
                context->workDoneCount++;
            }
        }
        else
        {
            // printf("Waiting for task in the queue!, %d\n", context->threadId);
            WaitForSingleObject(queue->semaphore, INFINITE);
        }
    }
    
    return 0;
}

WorkQueue* CreateWorkQueueAndWorkerThreads(unsigned int threadCount) 
{
    SIZE_T stackSize = 0;

    HANDLE semaphore = CreateSemaphore(0, 0, threadCount, 0);

    WorkQueue *workQueue = (WorkQueue*)malloc(sizeof(WorkQueue));
    workQueue->semaphore = semaphore;

    for(int n = 0; n < threadCount; n++)
    {
        ThreadContext *context = (ThreadContext*)malloc(sizeof(ThreadContext));
        context->threadId = n;
        context->queue = workQueue;
        context->workDoneCount = 0;
        HANDLE threadHandle = CreateThread(0, stackSize, (LPTHREAD_START_ROUTINE)DoWorkFromQueue, context, 0, 0);
    }

    return workQueue;
}

void WaitUntilCompletion(WorkQueue *workQueue) {
    while(TRUE)
    {
        boolean allCompleted = TRUE;
        for(int n = 0; n < workQueue->entryCount; n++)
        {
            if(!workQueue->entries[n].isCompleted)
            {
                allCompleted = FALSE;
                break;
            }
        }
        if(allCompleted) break;
    }
}
