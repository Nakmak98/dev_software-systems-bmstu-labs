/*
 * Программа предназначена для решения СЛАУ методом Гаусса.
 * Прямой ход метода Гаусса выполняют параллельные потоки,
 * которые ведут исключение элементов в прямом ходе по столбцам.
 *
 * Программа, также, считает чистое время решения уравнения,
 * исключая накладные расходы.
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define N 2048 //размерность матрицы

/*
 * Структура предназначена для хранения
 * необходимой информации для синхронизации потоков
 */
typedef struct {
    pthread_t tid; // идентификатор потока
    int start;     // начальная строка для исключения
    int step;      // шаг для определения следующей строки
} Threads;

void *thread_exclusion(void *arg_p);

void matrix_initialization();

void direct_move();

void reverse_move();

void print_matrix();

int time_stop();

struct timeval timeval1, timeval2, diff_timeval;
struct timezone tz;

pthread_barrier_t bar1;
pthread_barrier_t bar2;

double *A, *B, *X;

int main(int argc, char **argv) {
    int i;
    int threads_num = atoi(argv[1]);
    pthread_attr_t pattr;
    pthread_attr_init(&pattr);
    pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
    pthread_barrier_init(&bar1, NULL, threads_num + 1);
    pthread_barrier_init(&bar2, NULL, threads_num + 1);
    Threads *threads = (Threads *) calloc(threads_num, sizeof(Threads));
    A = (double *) calloc(N * N, sizeof(double));
    X = (double *) calloc(N, sizeof(double));
    B = (double *) calloc(N, sizeof(double));
    matrix_initialization();
    gettimeofday(&timeval1, &tz);
    for (i = 0; i < threads_num; i++) {
        threads[i].start = i;
        threads[i].step = threads_num;
        if (pthread_create(&threads[i].tid, &pattr, &thread_exclusion, &threads[i]))
            perror("pthread_create");
    }
    direct_move();
    reverse_move();
    for (i = N - 20; i < N; i++) {
        printf("%f\n", X[i]);
    }
    printf("Time: %d ms\n", time_stop());
    free(X);
    free(A);
    free(B);
    free(threads);
    pthread_barrier_destroy(&bar1);
    pthread_barrier_destroy(&bar2);
    pthread_attr_destroy(&pattr);
    return 0;
}

/*
 * Функция выполняет нормирование ведущей строки
 * Для синхронизации управляющих потоков
 * используются барьеры bar1, bar2.
 *
 * Когда управляющий поток main заканчивает обрабатывать ведущую строку,
 * он встает к барьеру bar1, таким образом запуская потоки.
 * Пока потоки выполняют исключения элементов в столбце,
 * поток main ждёт, когда потоки подойдут к барьеру bar2.
 */
void direct_move() {
    int i, j;
    double temp;
    for (i = 0; i < N; i++) {
        temp = A[N * i + i];
        for (j = i; j < N; j++) {
            A[N * i + j] /= temp;
        }
        B[i] /= temp;
        pthread_barrier_wait(&bar1);
        pthread_barrier_wait(&bar2);
    }
}

/*
 * Выполнение обратного хода Гаусса
 * Функция изменяет внешний массив X
 */
void reverse_move() {
    int i = 0, j = 0;
    for (i = N - 1; i >= 0; i--) {
        X[i] = B[i];
        for (j = N - 1; j > i; j--) {
            X[i] -= X[j] * A[i * N + j];
        }
    }
}

/*
 * Инициализирует внешний массив A случайными значениями от -1 до 1.
 * Элементы внешнего массива B есть сумма элементов строки матрицы А.
 */
void matrix_initialization() {
    int i, j;
    srand(time(NULL));
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            A[N * i + j] = ((double) rand() * 2 / RAND_MAX) - 1.0;
            B[i] += A[N * i + j];
        }
    }
}

// Функция выводит матрицу A и вектор B
void print_matrix() {
    int i, j;
    printf("Matrix A: \n");
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++)
            printf("%f ", A[N * i + j]);
        putchar('\n');
    }
    printf("\nMatrix B: \n");
    for (i = 0; i < N; i++) {
        printf("%f\n", B[i]);
    }
    putchar('\n');
}

/*
 * Функция которую выполняют потоки для исключения элементов
 * матрицы свободных коэффициентов по столбцам.
 */
void *thread_exclusion(void *threads) {
    int i, j, k;
    double temp;
    Threads *thread = (Threads *) threads;
    for (j = 0; j < N; j++) {
        if (j >= N - 1)
            break;
        pthread_barrier_wait(&bar1); //поток ожидает, когда main закончит работу с ведущей строкой
        for (i = j + 1 + thread->start; i < N; i += thread->step) { // определение строки, которую обрабатывает поток
            temp = A[N * i + j];
            B[i] = (B[i] / temp) - B[j];
            for (k = j; k < N; k++) {
                A[N * i + k] /= temp;
                A[N * i + k] -= A[N * j + k];
            }
        }
        pthread_barrier_wait(&bar2); // поток закончил обрабатывать столбец
        // поток main начинает работать со следующей ведущей строкой
    }
    pthread_barrier_wait(&bar1);
    pthread_barrier_wait(&bar2);
    pthread_exit(0);
}

/*
 * Функция считает чистое времени решения уравнения
 */
int time_stop() {
    gettimeofday(&timeval2, &tz);
    diff_timeval.tv_sec = timeval2.tv_sec - timeval1.tv_sec;
    diff_timeval.tv_usec = timeval2.tv_usec - timeval1.tv_usec;
    if (diff_timeval.tv_usec < 0) {
        diff_timeval.tv_sec--;
        diff_timeval.tv_usec += 1000000;
    }
    return diff_timeval.tv_sec * 1000 + diff_timeval.tv_usec / 1000;
}
