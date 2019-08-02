#ifndef MKR_GAUSSMULTITHREAD_H
#define MKR_GAUSSMULTITHREAD_H
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
/*
 * Структура предназначена для хранения
 * необходимой информации для синхронизации потоков
 */
typedef struct {
    pthread_t tid; // идентификатор потока
    int start;     // начальная строка для исключения
    int step;      // шаг для определения следующей строки
} Threads;

int gauss_solve(const double* A, double* X, const double* B, int N, int M);

void *thread_exclusion(void *arg_p);

void matrix_initialization(const double *, const double *);

void direct_move();

void reverse_move(double *);

pthread_barrier_t bar1;
pthread_barrier_t bar2;

double *gauss_A, *gauss_B, *gauss_X;
int N, M;
#endif //MKR_GAUSSMULTITHREAD_H
