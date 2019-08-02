#include <stdio.h>
#include <math.h>
#include "GaussMultithread.h"
#define N 3
#define M 2
double test_square_A[N * N] = {
       1.0, 2.0, 3.0,
       4.0, 5.0, 6.0,
       8.0, 12.0, 14.0
};

double test_non_square_A[N*M] = {
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0
};

double test_B[N] = {1.0, 2.0, 3.0};

int check_answer(double *answer){
   double expected[N] = {0.833333, -1.666667, 1.166667};
    for (int i = 0; i < N; ++i) {
         if (isnan(answer[i]))
             return -2;
         if (isinf(answer[i]))
             return -3;
         if (answer[i] != expected[i])
             return -1;
    }
    return 1;
}

int run(){
    double* X = (double *) calloc(N * N, sizeof(double));
    gauss_solve(test_square_A, X, test_B, N, N);
    check_answer(X);
    free(X);
    return 1;
}

int main(){
    if (!run()){
        exit(-1);
    }
}
