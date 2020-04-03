/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"


int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void sub_trans(int m, int n, int M, int N, int* A, int* B);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
   int block_size;
   int i,j;
   if(M==32 && N==32) {
      block_size = 8;
   } else if (M==64 && N==64) {
      // int subblock_size = 4;
      block_size = 4;
      // for (i=0; i<N; i+=block_size) {
      //    for (j=0; j<M; j+=block_size) {
      //
      //    }
      // }
      return;
   } else if (M==61 && N==67) {
      block_size = 23;
   } else { // Generalized case with blocksize of 8
      block_size = 8;
   }

   for (i=0; i<N; i+=block_size) {
      for (j=0; j<M; j+=block_size) {
         sub_trans(j+block_size <= M ? block_size: M % block_size,
                      i+block_size <= N ? block_size: N % block_size,
                      M, N, &A[i][j], &B[j][i]);
      }
   }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * sub_trans - a regular naive transpose, but takes additional arguments so
 *             that it can be used to transpose a submatrix of a larger matrix
 * m and n are the size of the submatrix where M and N are the size of the
 * larger matrix.
 * A and B must point to the starting location of the submatrix
 */
void sub_trans(int m, int n, int M, int N, int* A, int* B) {
   int i, j;
   int d,tmp;
   for(i=0; i<n; ++i) {
      for(j=0; j<m; ++j) {
         if(i!=j)
            B[N*j+i] = A[M*i+j];
         else {
            tmp=A[M*i+j];
            d = j;
         }
      }
      B[N*d+d] = tmp;
   }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
