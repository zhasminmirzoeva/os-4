#include <stdlib.h>
#include <stdio.h>

// Check if name (ID) is unique
int is_unique(int name, int *arr, int size)
{
    for (int i = 0; i < size; ++i)
    {
        if (arr[i] == name)
        {
            return 0;
        }
    }
    return 1;
}

// Fill the array with unique random names (IDs) 
void gen_unique_names(int *arr, int size)
{
    for (int i = 0; i < size; ++i)
    {
        int name = rand();
        while (!is_unique(name, arr, i))
        {
            name = rand();
        }
        arr[i] = name;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <M> <N> <K> <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const int M = atoi(argv[1]);
    const int N = atoi(argv[2]);    
    const int K = atoi(argv[3]);
    const char *filename = argv[4];

    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Unable to create file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    // Create array for book's names
    int *arr = malloc(M* N * K * sizeof(*arr));
    // Generate unique names
    gen_unique_names(arr, M * N * K);

    // Output generated names and positions
    for (int m = 0; m < M; ++m)    
    {
        for (int n = 0; n < N; ++n)
        {
            for (int k = 0; k < K; ++k)
            {
                int idx = k + n * K + m * K * N;
                fprintf(fp, "%d:%d:%d:%d\n", m, n, k, arr[idx]);
                printf("%d - %d, %d, %d\n", arr[idx], m, n, k);
            }
        }
    }

    free(arr);
    fclose(fp);

    return EXIT_SUCCESS;
}
