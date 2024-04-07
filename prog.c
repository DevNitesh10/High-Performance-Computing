/*
 * To compile the program:
 *  - gcc prog.c -o prog -lpthread
 *
 * To execute the compiled program
 *  - ./prog <input_file> <num_threads> for e.g ./prog MatData.txt 4
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Structure to represent a matrix
typedef struct
{
    int rows, cols;
    double **data;
} Matrix;

// Structure to hold data for each thread
typedef struct
{
    int start_row;
    int end_row;
    Matrix *matrix_A, *matrix_B, *result_matrix;
} ThreadData;

// Function to allocate memory for a matrix
Matrix *allocateMatrix(int rows, int cols)
{
    Matrix *matrix = (Matrix *)malloc(sizeof(Matrix));
    if (matrix == NULL)
    {
        return NULL;
    }
    matrix->rows = rows;
    matrix->cols = cols;
    matrix->data = (double **)malloc(rows * sizeof(double *));
    for (int i = 0; i < rows; i++)
    {
        matrix->data[i] = (double *)malloc(cols * sizeof(double));
    }

    return matrix;
}

// Function to read a matrix from a file
Matrix *readMatrix(FILE *file)
{
    int rows = -1, cols = -1;
    fscanf(file, "%d,%d", &rows, &cols);
    if (rows == -1 || cols == -1)
    {
        return NULL;
    }

    Matrix *matrix = allocateMatrix(rows, cols);
    if (matrix == NULL)
    {
        printf("Error allocating matrix");
        return NULL;
    }

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            fscanf(file, "%lf,", &matrix->data[i][j]);
        }
    }

    return matrix;
}

// Function to free memory occupied by a matrix
void freeMatrix(Matrix *matrix)
{
    for (int i = 0; i < matrix->rows; i++)
    {
        free(matrix->data[i]);
    }
    free(matrix->data);
    free(matrix);
}

void *multiplyRows(void *arg)
{
    ThreadData *data = (ThreadData *)arg;

     // Multiply rows of matrix A by matrix B and store the result in the result matrix
    for (int i = data->start_row; i < data->end_row; i++)
    {
        for (int j = 0; j < data->matrix_B->cols; j++)
        {
            data->result_matrix->data[i][j] = 0;
            for (int k = 0; k < data->matrix_A->cols; k++)
            {
                data->result_matrix->data[i][j] += data->matrix_A->data[i][k] * data->matrix_B->data[k][j];
            }
        }
    }
}

void writeMatrix(FILE *file, Matrix *matrix)
{
     // Write the matrix dimensions to the file
    fprintf(file, "%d,%d\n", matrix->rows, matrix->cols);

    // Write the matrix elements to the file
    for (int i = 0; i < matrix->rows; i++)
    {
        for (int j = 0; j < matrix->cols; j++)
        {
            // Add a comma after each element (except the last one in a row)
            fprintf(file, "%.6lf", matrix->data[i][j]);
            if (j != matrix->cols - 1)
                fprintf(file, ",");
        }
        // Add a new line after each row
        fprintf(file, "\n");
    }
    // Add an extra newline for better readability
    fprintf(file, "\n");
}

int main(int argc, char *argv[])
{
    /*
     * Verify the arguments
     */
    if (argc != 3)
    {
        printf("Usage: '%s' <input_file> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /*
     * Open the data file in read mode
     */
    FILE *data_file = fopen(argv[1], "r");
    if (data_file == NULL)
    {
        printf("Error opening file %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    /*
     * Get the number of threads to use
     */
    int num_threads = atoi(argv[2]);
    if (num_threads <= 0)
    {
        printf("Invalid number of threads.\n");
        fclose(data_file);
        return EXIT_FAILURE;
    }

    /*
     * Open output.txt
     */
    FILE *output_file = fopen("output.txt", "w");
    if (output_file == NULL)
    {
        printf("Error creating output file %s\n", argv[1]);
        fclose(data_file);
        return EXIT_FAILURE;
    }

    // Infinite loop to read matrices from the input file
    while (1)
    {
        // Read two matrices from the input file
        Matrix *matrix_A = readMatrix(data_file);
        Matrix *matrix_B = readMatrix(data_file);

         // Break the loop if no more matrices can be read
        if (matrix_A == NULL || matrix_B == NULL)
        {
            break;
        }

        // Check if matrices can be multiplied
        if (matrix_A->cols != matrix_B->rows)
        {
            printf("Error: Matrices cannot be multiplied.\n");
            freeMatrix(matrix_B);
            freeMatrix(matrix_A);
            continue;
        }

        // Allocate result matrix
        Matrix *result_matrix = allocateMatrix(matrix_A->rows, matrix_B->cols);
        if (result_matrix == NULL)
        {
            printf("Error allocating result matrix");
        }

        // Limiting number of threads if required
        num_threads = (num_threads > matrix_A->rows) ? matrix_A->rows : num_threads;

        // Calculate the number of rows each thread will handle
        int rows_per_thread = matrix_A->rows / num_threads;
        int remaining_rows = matrix_A->rows % num_threads;

        // Create threads
        pthread_t threads[num_threads];
        ThreadData thread_data[num_threads];
        int start_row = 0;
        for (int i = 0; i < num_threads; i++)
        {
            // Assign thread data
            thread_data[i].start_row = start_row;
            thread_data[i].end_row = start_row + rows_per_thread + (i < remaining_rows ? 1 : 0);
            thread_data[i].matrix_A = matrix_A;
            thread_data[i].matrix_B = matrix_B;
            thread_data[i].result_matrix = result_matrix;

            // Create a thread to multiply rows
            pthread_create(&threads[i], NULL, multiplyRows, (void *)&thread_data[i]);
            
            // Update the starting row for the next thread
            start_row = thread_data[i].end_row;
        }

        // Wait for threads to finish
        for (int i = 0; i < num_threads; i++)
        {
            pthread_join(threads[i], NULL);
        }

        // Write the result matrix to an output file
        writeMatrix(output_file, result_matrix);

        // Clean up the allocated memory
        freeMatrix(matrix_A);
        freeMatrix(matrix_B);
        freeMatrix(result_matrix);
    }

    // Close the opened files
    fclose(data_file);
    fclose(output_file);

    return EXIT_SUCCESS;
}