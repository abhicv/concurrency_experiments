#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>

#include "matrix.c"
#include "workqueue.c"

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

Matrix MatrixMultiplyConcurrent(Matrix matrixA, Matrix matrixB, WorkQueue *queue)
{    
    if(matrixA.column != matrixB.row)
    {
        printf("error: matrix a column count(%d) should be equal to matric b row count(%d)", matrixA.column, matrixB.row);
        return (Matrix){0};
    }

    Matrix result = CreateMatrix(matrixA.row, matrixB.column);

    WorkData *datas = calloc(matrixA.row * matrixB.column, sizeof(WorkData));

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
            entry.isCompleted = false;

            PushWork(queue, entry);
        }
    }

    // making the calling thread busy until all the submitted work is finished 
    // TODO: block the thread instead of making it busy and consuming cpu time
    WaitUntilCompletion(queue);

    return result;    
}

#define THREAD_COUNT 8

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
    // PrintMatrix(matrixB);
    // WriteMatrixToFile(matrixB, "matrixB.log");

    #if 1
    QueryPerformanceCounter(&start);
    Matrix result1 = MatrixMultiply(matrixA, matrixB);
    QueryPerformanceCounter(&end);
    normalTime = end.QuadPart - start.QuadPart;
    printf("normal time: %lld\n", normalTime);
    // PrintMatrix(result1);
    // WriteMatrixToFile(result1, "result1.log");
    #endif

    WorkQueue *workQueue = CreateWorkQueueAndWorkerThreads(THREAD_COUNT);

    #if 1
    QueryPerformanceCounter(&start);
    Matrix result2 = MatrixMultiplyConcurrent(matrixA, matrixB, workQueue);
    QueryPerformanceCounter(&end);
    unsigned long long concurrentTime = end.QuadPart - start.QuadPart;
    // WriteMatrixToFile(result2, "result2.log");
    // PrintMatrix(result2);

    // int workCount = 0;
    // for(int n = 0; n < THREAD_COUNT; n++) 
    // {
    //     workCount += contexts[n].workDoneCount;
    // }

    // printf("work done: %d\n", workCount);

    printf("concurrent time: %lld\n", concurrentTime);
    printf("speed gain: %f\n", (double) normalTime / (double) concurrentTime);

    long totalWork = (long)result2.row * (long)result2.column;

    #endif
    
    // for(int n = 0; n < THREAD_COUNT; n++)
    // {
    //     float percentage = (float)contexts[n].workDoneCount / (float)totalWork;
    //     printf("Work done by thread %u: %u / %ld (%0.2f %%)", contexts[n].threadId, contexts[n].workDoneCount, totalWork, percentage * 100.0f);
    //     printf("[");
    //     int percentRounded = (int)(percentage * 100.0f);
    //     for(int index = 0; index < 100; index++)
    //     {
    //         if(index <= percentRounded) printf("%c", 219);
    //         else printf("%c", 176);
    //     }
    //     printf("]\n");
    // }

    return 0;
}
