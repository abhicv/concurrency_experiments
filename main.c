#include <stdio.h>
#include <Windows.h>

#include "matrix.c"

typedef void* (*CallBack)(void *data);
typedef struct WorkQueueEntry
{
    CallBack callback;
    void *data;
    void *result;
    boolean isCompleted;
} WorkQueueEntry;

#define MAX_QUEUE_SIZE 10000000

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
    // printf("Thread %d spwaned\n", context->threadId);

    WorkQueue *queue = context->queue;
    while(1)
    {
        unsigned int entryTodo = queue->nextEntryTodo;
        if(entryTodo < queue->entryCount)
        {
            unsigned int index = InterlockedCompareExchange((long volatile*)&queue->nextEntryTodo, entryTodo + 1, entryTodo);
            if(index == entryTodo)
            {
                // printf("Work %d done on thread %d\n", entryTodo, context->threadId);
                WorkQueueEntry *entry = &queue->entries[entryTodo];
                entry->result = entry->callback(entry->data);
                entry->isCompleted = TRUE;
                context->workDoneCount++;
            }
        }
        else
        {
            // Sleep(1000);
            WaitForSingleObject(queue->semaphore, INFINITE);
        }
    }
    
    return 0;
}

Matrix MatrixMultiplyConcurrent(Matrix matrixA, Matrix matrixB, WorkQueue *queue)
{    
    if(matrixA.column != matrixB.row)
    {
        printf("error: matrix a column count(%d) should be equal to matric b row count(%d)", matrixA.column, matrixB.row);
        return (Matrix){0};
    }

    Matrix result = CreateMatrix(matrixA.row, matrixB.column);

    WorkData *datas = calloc(matrixA.row * matrixB.column, sizeof(WorkData));

    long progress = 0;
    for(int rowIndex = 0; rowIndex < matrixA.row; rowIndex++)
    {
        for(int columnIndex = 0; columnIndex < matrixB.column; columnIndex++)
        {
            unsigned int index = rowIndex * result.column + columnIndex;

            WorkData *data = &datas[index];
            data->A = &matrixA;
            data->B = &matrixB;
            data->result = &result;
            data->rowIndex = rowIndex;
            data->columnIndex = columnIndex;

            WorkQueueEntry entry = {0};
            entry.callback = RowColumnMultiplicationWork;
            entry.data = data;
            entry.isCompleted = FALSE;

            PushWork(queue, entry);
            // printf("progress: %ld / %ld\n", progress, result.row * result.column);
            // progress++;
        }
    }

    while(TRUE)
    {
        boolean allCompleted = TRUE;
        for(int n = 0; n < queue->entryCount; n++)
        {
            if(!queue->entries[n].isCompleted)
            {
                allCompleted = FALSE;
                break;
            }
        }
        if(allCompleted) break;
    }

    return result;    
}

static WorkQueue workQueue = {0};

int main(int argc, char **argv)
{
    LARGE_INTEGER start = {0};
    LARGE_INTEGER end = {0};
    unsigned long long normalTime = 0;

    unsigned int size = 1000;

    Matrix matrixA = MakeRandomValueMatrix(size, size);
    // PrintMatrix(matrixA);
    // WriteMatrixToFile(matrixA, "matrixA.log");
    
    Matrix matrixB = MakeRandomValueMatrix(size, size);
    // WriteMatrixToFile(matrixB, "matrixB.log");
    // PrintMatrix(matrixB);

    #if 1
    QueryPerformanceCounter(&start);
    Matrix result1 = MatrixMultiply(matrixA, matrixB);
    QueryPerformanceCounter(&end);
    normalTime = end.QuadPart - start.QuadPart;
    printf("normal time: %lld\n", normalTime);
    // PrintMatrix(result1);
    // WriteMatrixToFile(result1, "result1.log");
    #endif

    // workQueue.entries = (WorkQueueEntry*)calloc(MAX_QUEUE_SIZE, sizeof(WorkQueueEntry));

    SIZE_T stackSize = 0;

    #define THREAD_COUNT 8

    HANDLE semaphore = CreateSemaphore(0, 0, THREAD_COUNT, 0);

    workQueue.semaphore = semaphore;

    ThreadContext contexts[THREAD_COUNT] = {0};
    for(int n = 0; n < THREAD_COUNT; n++)
    {
        ThreadContext *context = &contexts[n];
        context->threadId = n;
        context->queue = &workQueue;
        context->workDoneCount = 0;
        HANDLE threadHandle = CreateThread(0, stackSize, DoWorkFromQueue, context, 0, 0);
    }

    #if 1
    QueryPerformanceCounter(&start);
    Matrix result2 = MatrixMultiplyConcurrent(matrixA, matrixB, &workQueue);
    QueryPerformanceCounter(&end);
    unsigned long long concurrentTime = end.QuadPart - start.QuadPart;
    // WriteMatrixToFile(result2, "result2.log");
    // PrintMatrix(result2);
    printf("concurrent time: %lld\n", concurrentTime);

    printf("speed gain: %f\n", (double) normalTime / (double)concurrentTime);

    long totalWork = (long)result2.row * (long)result2.column;
    for(int n = 0; n < THREAD_COUNT; n++)
    {
        float percentage = (float)contexts[n].workDoneCount / (float)totalWork;
        printf("Work done by thread %u: %u / %ld (%0.2f %%) ", contexts[n].threadId, contexts[n].workDoneCount, totalWork, percentage * 100.0f);
        printf("[");
        int percentRounded = (int)(percentage * 100.0f);
        for(int index = 0; index < 100; index++)
        {
            if(index <= percentRounded) printf("%c", 219);
            else printf("%c", 176);
        }
        printf("]\n");
    }

    #endif

    // Sleep(30000);

    return 0;
}
