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
#define BLOCK_SIZE 8
#define ALT_BLOCK 23

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void sub_trans(int m, int n, int M, int N, int* A, int* B);
void sub_trans8(int i, int j, int M, int N, int A[N][M], int B[M][N]);

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
   int i,j;

   if (M==64 && N==64) {
      for (i=0; i<N; i+=BLOCK_SIZE) {
         for (j=0; j<M; j+=BLOCK_SIZE) {
            sub_trans8(i,j, M, N, A, B);
         }
      }
      return;
   } else if (M==61 && N==67) {
      // block size of 23 from experimentation
      for (i=0; i<N; i+=ALT_BLOCK) {
         for (j=0; j<M; j+=ALT_BLOCK) {
            sub_trans(j+ALT_BLOCK <= M ? ALT_BLOCK: M % ALT_BLOCK,
                         i+ALT_BLOCK <= N ? ALT_BLOCK: N % ALT_BLOCK,
                         M, N, &A[i][j], &B[j][i]);
         }
      }
      return;
   }

   for (i=0; i<N; i+=BLOCK_SIZE) {
      for (j=0; j<M; j+=BLOCK_SIZE) {
         sub_trans(j+BLOCK_SIZE <= M ? BLOCK_SIZE: M % BLOCK_SIZE,
                      i+BLOCK_SIZE <= N ? BLOCK_SIZE: N % BLOCK_SIZE,
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
 * sub_trans - Transposes a matrix, but takes additional arguments so
 *             that it can be used to transpose a submatrix of a larger matrix.
 *             The algorithm will skip diagonals and write them after finishin
 *             a row.
 *
 * m and n are the size of the submatrix where M and N are the size of the
 * larger matrix.
 * A and B must point to the starting location of the submatrix
 */
void sub_trans(int m, int n, int M, int N, int* A, int* B) {
   int i, j;
   int d,tmp; // variables to store the diagonals
   for(i=0; i<n; ++i) {
      for(j=0; j<m; ++j) {
         if(i!=j)
            B[N*j+i] = A[M*i+j];
         else { // skipping the diagonals until later
            tmp=A[M*i+j];
            d = j;
         }
      }
      B[N*d+d] = tmp;
   }
}

/*
 * Performs transpose on an 8x8 submatrix using two 4x8 matrices
 * The matrix is notated in quadrants defined below
 *   1 2
 *   3 4
 * The letter preceeding the number is the matrix to which is belongs
 */
void sub_trans8(int i, int j, int M, int N, int A[N][M], int B[M][N]) {
   int r,c; // row and column relative to A
   // 8 temporary variables (acts as array)
   int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

   // iterating over top 4x8 matrix
   for(r=i; r<i+4; ++r) {
      // Storing the rows of A1 and A2
      tmp1 = A[r][j];
      tmp2 = A[r][j+1];
      tmp3 = A[r][j+2];
      tmp4 = A[r][j+3];
      tmp5 = A[r][j+4];
      tmp6 = A[r][j+5];
      tmp7 = A[r][j+6];
      tmp8 = A[r][j+7];

      // these 4 are in the correct locations (B1)
      B[j][r] = tmp1;
      B[j+1][r] = tmp2;
      B[j+2][r] = tmp3;
      B[j+3][r] = tmp4;

      // these 4 should go in B3 but, will be stored in B2 temp
      B[j][r+4] = tmp5;
      B[j+1][r+4] = tmp6;
      B[j+2][r+4] = tmp7;
      B[j+3][r+4] = tmp8;
   }

   for(c=j; c<j+4; ++c) {
      // storing A3
      tmp5 = A[i+4][c];
      tmp6 = A[i+5][c];
      tmp7 = A[i+6][c];
      tmp8 = A[i+7][c];

      // storing B2 (wrong values stored there)
      tmp1 = B[c][i+4];
      tmp2 = B[c][i+5];
      tmp3 = B[c][i+6];
      tmp4 = B[c][i+7];

      // writing the correct values to B2
      B[c][i+4] = tmp5;
      B[c][i+5] = tmp6;
      B[c][i+6] = tmp7;
      B[c][i+7] = tmp8;

      // writing correct values to B3
      B[c+4][i] = tmp1;
      B[c+4][i+1] = tmp2;
      B[c+4][i+2] = tmp3;
      B[c+4][i+3] = tmp4;

      // writing correct values from A4 to B4
      for(r=i+4; r<i+8; ++r) {
         B[c+4][r] = A[r][c+4];
      }
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
