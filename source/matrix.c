#include <stdio.h>
#include <Windows.h>

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
