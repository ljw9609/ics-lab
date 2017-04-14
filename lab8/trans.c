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

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
void test_32(int M, int N, int A[N][M], int B[M][N]);
void test_64(int M, int N, int A[N][M], int B[M][N]);
void test_61(int M, int N, int A[N][M], int B[M][N]);

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]){
	if(M == 32){
		test_32(M,N,A,B);
	}
	else if(M == 64){
		test_64(M,N,A,B);
	}
	else if(M == 61){
		test_61(M,N,A,B);
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

/* transpose test for 32x32,succeed */
char test_trans_32[] = "test trans 32";
void test_32(int M,int N,int A[N][M],int B[M][N]){
	int tmp0,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7;
	for(int j = 0;j < N;j+=8){
		for(int i = 0;i < M;i++){
			tmp0 = A[i][j];
			tmp1 = A[i][j+1];
			tmp2 = A[i][j+2];
			tmp3 = A[i][j+3];
			tmp4 = A[i][j+4];
			tmp5 = A[i][j+5];
			tmp6 = A[i][j+6];
			tmp7 = A[i][j+7];
			B[j][i]   = tmp0;
			B[j+1][i] = tmp1;
			B[j+2][i] = tmp2;
			B[j+3][i] = tmp3;
			B[j+4][i] = tmp4;
			B[j+5][i] = tmp5;
			B[j+6][i] = tmp6;
			B[j+7][i] = tmp7;
		}
	}
}

char test_trans_64[] = "test trans 64";
void test_64(int M,int N,int A[N][M],int B[M][N]){
	int tmp0,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7;
	for (int i = 0; i < 64; i += 8)
      for (int j = 0; j < 64; j += 8){
        for (int k = j; k < j+4; k++){
          tmp0 = A[k][i];
          tmp1 = A[k][i+1];
          tmp2 = A[k][i+2];
          tmp3 = A[k][i+3];
          tmp4 = A[k][i+4];//save data to B temporarily
          tmp5 = A[k][i+5];//
          tmp6 = A[k][i+6];//
          tmp7 = A[k][i+7];//
          B[i][k] = tmp0;
          B[i+1][k] = tmp1;
          B[i+2][k] = tmp2;
          B[i+3][k] = tmp3;
          B[i][k+4] = tmp4;
          B[i+1][k+4] = tmp5;
          B[i+2][k+4] = tmp6;
          B[i+3][k+4] = tmp7;
        }
        for (int k = i; k < i+4; k++){
          tmp0 = B[k][j+4];//get saved data before
          tmp1 = B[k][j+5];//
          tmp2 = B[k][j+6];//
          tmp3 = B[k][j+7];//
          B[k][j+4] = A[j+4][k];
          B[k][j+5] = A[j+5][k];
          B[k][j+6] = A[j+6][k];
          B[k][j+7] = A[j+7][k];
          B[k+4][j] = tmp0;
          B[k+4][j+1] = tmp1;
          B[k+4][j+2] = tmp2;
          B[k+4][j+3] = tmp3;
        }
        for (int k = i+4; k < i+8; k++){
          tmp0 = A[j+4][k];
          tmp1 = A[j+5][k];
          tmp2 = A[j+6][k];
          tmp3 = A[j+7][k];
          B[k][j+4] = tmp0;
          B[k][j+5] = tmp1;
          B[k][j+6] = tmp2;
          B[k][j+7] = tmp3;
        }
    }
}

char test_trans_612[] = "test trans 612";
void test_612(int M,int N,int A[N][M],int B[M][N]){
	int tmp;	//find on the internet
	for (int i = 0; i < 61; i += 17)
      for (int j = 0; j < 67; j+= 17)
        for (int k = j; k < j+17 && k < 67; ++k)
          for (int m = i; m < i+17 && m < 61; ++m){
            tmp = A[k][m];
            B[m][k] = tmp;
          }
}

char test_trans_61[] = "test trans 61";
void test_61(int M,int N,int A[N][M],int B[M][N]){
	int tmp0,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7;
	int i,j,k;
	for (k = 0; k < 64; k+=8){
           for (j = 0; j < 60; j+=6){  
                for (i = k; i < k+4; i++){
                     tmp0 = A[i][j];
                     tmp1 = A[i][j+1];
                     tmp2 = A[i][j+2];
                     tmp3 = A[i][j+3];
                     tmp4 = A[i][j+4];
                     tmp5 = A[i][j+5];                
                     B[j][i] = tmp0;
                     B[j+1][i] = tmp1;
                     B[j+2][i] = tmp2;
                     B[j][i+4] = tmp3;
                     B[j+1][i+4] = tmp4;
                     B[j+2][i+4] = tmp5;
                     if (j == 54){
                        tmp0 = A[i][60];
                        B[60][i] = tmp0;
                    }
                }
                 for (i = j; i < j+3; i++){                
                     tmp0 = B[i][k+4];
                     tmp1 = B[i][k+5];
                     tmp2 = B[i][k+6];
                     tmp7 = B[i][k+7];
                     tmp3 = A[k+4][i];
                     tmp4 = A[k+5][i];
                     tmp5 = A[k+6][i];
                     tmp6 = A[k+7][i];
                     B[i][k+4] = tmp3;
                     B[i][k+5] = tmp4;
                     B[i][k+6] = tmp5;
                     B[i][k+7] = tmp6;
                     B[i+3][k] = tmp0;
                     B[i+3][k+1] = tmp1;
                     B[i+3][k+2] = tmp2;
                     B[i+3][k+3] = tmp7;
                  }
                  for (i = j + 3; i < j+6; i++){                 
                      tmp0 = A[k+4][i];
                      tmp1 = A[k+5][i];
                      tmp2 = A[k+6][i];
                      tmp3 = A[k+7][i];
                      B[i][k+4] = tmp0;
                      B[i][k+5] = tmp1;
                      B[i][k+6] = tmp2;
                      B[i][k+7] = tmp3;
                      if (i == 59){                      
                        tmp0 = A[k+4][i+1];
                        tmp1 = A[k+5][i+1];
                        tmp2 = A[k+6][i+1];
                        tmp3 = A[k+7][i+1];
                        B[i+1][k+4] = tmp0;
                        B[i+1][k+5] = tmp1;
                        B[i+1][k+6] = tmp2;
                        B[i+1][k+7] = tmp3;
                    	}   
                    }
                  if (k == 56){                
                     for (i = j;i < j+6; i++){                 
                         tmp0 = A[64][i];
                         tmp1 = A[65][i];
                         tmp2 = A[66][i];
                         B[i][64] = tmp0;
                         B[i][65] = tmp1;
                         B[i][66] = tmp2;
                         if (i == 59 ){                       
                            tmp0 = A[64][60];
                            tmp1 = A[65][60];
                            tmp2 = A[66][60];
                            B[60][64] = tmp0;
                            B[60][65] = tmp1;
                            B[60][66] = tmp2;
                         }
                     }
                }
           }   
       }
}
/*
char test_trans_321[] = "test trans 321";
void test_321(int M,int N,int A[N][M],int B[M][N]){
	int tmp0,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7;
	for(int i = 0;i < N;i++){
		for(int j = 0;j < M;j+=8){
			tmp0 = A[i][j];
			tmp1 = A[i][j+1];
			tmp2 = A[i][j+2];
			tmp3 = A[i][j+3];
			tmp4 = A[i][j+4];
			tmp5 = A[i][j+5];
			tmp6 = A[i][j+6];
			tmp7 = A[i][j+7];
			B[j][i]   = tmp0;
			B[j+1][i] = tmp1;
			B[j+2][i] = tmp2;
			B[j+3][i] = tmp3;
			B[j+4][i] = tmp4;
			B[j+5][i] = tmp5;
			B[j+6][i] = tmp6;
			B[j+7][i] = tmp7;
		}
	}
}
*/
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
    //registerTransFunction(trans, trans_desc); 
    //registerTransFunction(test_32,test_trans_32);
    //registerTransFunction(test_64,test_trans_64);
    //registerTransFunction(test_61,test_trans_61);
    //registerTransFunction(test_612,test_trans_612);

    //registerTransFunction(test_321,test_trans_321);

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

