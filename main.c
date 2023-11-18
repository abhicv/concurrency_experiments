#include <stdio.h>
#include <Windows.h>

typedef void* (*CallBack)(void *data);
typedef struct WorkQueueEntry
{
    CallBack callback;
    void *data;
    void *result;
    boolean isCompleted;
} WorkQueueEntry;

#define MAX_QUEUE_SIZE 1000000000

typedef struct WorkQueue
{
    // WorkQueueEntry entries[MAX_QUEUE_SIZE];
    WorkQueueEntry *entries;
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

    // TODO: write memory fence
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

static WorkQueue workQueue = {0};

typedef struct Matrix {
    unsigned int row;
    unsigned int column;
    float *data;
} Matrix;

Matrix CreateMatrix(unsigned int row, unsigned int column)
{
    Matrix matrix = {0};
    matrix.row = row;
    matrix.column = column;
    matrix.data = (float*)calloc(row * column, sizeof(float));
    return matrix;
}

Matrix MakeRandomValueMatrix(unsigned int row, unsigned int column)
{
    Matrix matrix = CreateMatrix(row, column);

    for(int n = 0; n < (row * column); n++)
    {
        matrix.data[n] = (float)rand() / (float)RAND_MAX;
    }

    return matrix;
}

float MultiplyAddRowColumn(Matrix matrixA, Matrix matrixB, unsigned int rowIndex, unsigned int columnIndex)
{
    float result = 0.0f;
    for(int n = 0; n < matrixA.column; n++)
    {
        result += (matrixA.data[rowIndex * matrixA.column + n] * matrixB.data[n * matrixB.column + columnIndex]);
    }
    return result;
}

typedef struct WorkData
{
    Matrix *result;
    Matrix *A;
    Matrix *B;
    unsigned int rowIndex;
    unsigned int columnIndex;
} WorkData;

void *RowColumnMultiplicationWork(void *data)
{
    WorkData *workData = (WorkData*)data;
    Matrix *result = workData->result;
    result->data[workData->rowIndex * result->column + workData->columnIndex] = MultiplyAddRowColumn(*workData->A, *workData->B, workData->rowIndex, workData->columnIndex);
    return &result->data[workData->rowIndex * result->column + workData->columnIndex];
}

Matrix MatrixMultiply(Matrix matrixA, Matrix matrixB)
{    
    if(matrixA.column != matrixB.row)
    {
        printf("error: matrix a column count(%d) should be equal to matric b row count(%d)", matrixA.column, matrixB.row);
        return (Matrix){0};
    }

    Matrix result = CreateMatrix(matrixA.row, matrixB.column);

    for(int rowIndex = 0; rowIndex < matrixA.row; rowIndex++)
    {
        for(int columnIndex = 0; columnIndex < matrixB.column; columnIndex++)
        {
            unsigned int index = rowIndex * result.column + columnIndex;
            float sum = MultiplyAddRowColumn(matrixA, matrixB, rowIndex, columnIndex);
            result.data[index] = sum;            
        }
    }

    return result;    
}

Matrix MatrixMultiplyConcurrent(Matrix matrixA, Matrix matrixB)
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

            PushWork(&workQueue, entry);
            // printf("progress: %ld / %ld\n", progress, result.row * result.column);
            // progress++;
        }
    }

    while(1)
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

    return result;    
}

void PrintMatrix(Matrix matrix)
{
    if(matrix.data == 0) return;

    printf("\n");
    for(int rowIndex = 0; rowIndex < matrix.row; rowIndex++)
    {
        printf("[ ");
        for(int columnIndex = 0; columnIndex < matrix.column; columnIndex++)
        {
            printf("%f, ", matrix.data[rowIndex * matrix.column + columnIndex]);
        }
        printf("]\n");
    }
    printf("\n");
}

void WriteMatrixToFile(Matrix matrix, char *fileName)
{
    if(matrix.data == 0) return;

    FILE *file = fopen(fileName, "w+");

    fprintf(file, "\n");
    for(int rowIndex = 0; rowIndex < matrix.row; rowIndex++)
    {
        fprintf(file, "[ ");
        for(int columnIndex = 0; columnIndex < matrix.column; columnIndex++)
        {
            fprintf(file, "%f, ", matrix.data[rowIndex * matrix.column + columnIndex]);
        }
        fprintf(file, "]\n");
    }
    fprintf(file, "\n");
}

void *SimplePrintWork(void *data)
{
    return 0;
}

int main(int argc, char **argv)
{
    LARGE_INTEGER start = {0};
    LARGE_INTEGER end = {0};
    long normalTime = 0;

    unsigned int size = 4;

    Matrix matrixA = MakeRandomValueMatrix(size, size);
    // PrintMatrix(matrixA);
    // WriteMatrixToFile(matrixA, "matrixA.log");
    
    Matrix matrixB = MakeRandomValueMatrix(size, size);
    // WriteMatrixToFile(matrixB, "matrixB.log");
    // PrintMatrix(matrixB);

    #if 0
    QueryPerformanceCounter(&start);
    Matrix result1 = MatrixMultiply(matrixA, matrixB);
    QueryPerformanceCounter(&end);
    normalTime = end.QuadPart - start.QuadPart;
    printf("normal time: %ld\n", normalTime);
    PrintMatrix(result1);
    WriteMatrixToFile(result1, "result1.log");
    #endif

    workQueue.entries = (WorkQueueEntry*)calloc(MAX_QUEUE_SIZE, sizeof(WorkQueueEntry));

    SIZE_T stackSize = 0;

    #define THREAD_COUNT 4

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

    #if 0
    WorkQueueEntry entry = {0};
    entry.callback = SimplePrintWork;
    entry.isCompleted = FALSE;

    PushWork(&workQueue, entry);
    PushWork(&workQueue, entry);
    PushWork(&workQueue, entry);
    PushWork(&workQueue, entry);
    PushWork(&workQueue, entry);
    PushWork(&workQueue, entry);
    PushWork(&workQueue, entry);
    PushWork(&workQueue, entry);
    PushWork(&workQueue, entry);

    while(1) {Sleep(1000);}
    #endif

    #if 1

    Sleep(1000);

    QueryPerformanceCounter(&start);
    Matrix result2 = MatrixMultiplyConcurrent(matrixA, matrixB);
    QueryPerformanceCounter(&end);
    long concurrentTime = end.QuadPart - start.QuadPart;
    // WriteMatrixToFile(result2, "result2.log");
    // PrintMatrix(result2);
    printf("concurrent time: %ld\n", concurrentTime);

    printf("speed gain: %f\n", (float) normalTime / (float)concurrentTime);

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

    Sleep(30000);

    return 0;
}
