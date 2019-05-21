/*
 * Библиотека предназначена для решения СЛАУ методом Гаусса.
 * Прямой ход метода Гаусса выполняют параллельные потоки,
 * которые ведут исключение элементов в прямом ходе по столбцам.
 *
 * Компиляция: сс <ваша прикладная программа> GaussMultithread.c -lpthread
 * ВАЖНО: подключите заголовочный файл GaussMultithread.h
 */

#include "GaussMultithread.h"

/*
 * Функция-решатель. Создает потоки управления (4), которые ведут исключение элементов по столбцам
 * Поток main ответственен за выбор ведущей строки и столбца.
 * */
int gauss_solve(const double *A, double *X, const double *B, int n, int m) {
    /*
     *  const double *A - матрица коэффициентов (в виде одномерного массива)
     *  double *B       - вектор свободных членов
     *  const double *X - вектор неизвестных.
     *  n - количество неизвестных
     *  m - количество уравнений
     * */
    const int threads_num = 4;
    N = n;
    M = m;
    pthread_attr_t pattr;
    pthread_attr_init(&pattr);
    pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
    pthread_barrier_init(&bar1, NULL, threads_num + 1);
    pthread_barrier_init(&bar2, NULL, threads_num + 1);
    Threads *threads = (Threads *) calloc(threads_num, sizeof(Threads));
    gauss_A = (double *) calloc(N * M, sizeof(double));
    gauss_B = (double *) calloc(N, sizeof(double));
    matrix_initialization(A, B);
    for (int i = 0; i < threads_num; i++) {
        threads[i].start = i;
        threads[i].step = threads_num;
        if (pthread_create(&threads[i].tid, &pattr, &thread_exclusion, &threads[i]))
            perror("Error: pthread_create() return exit status;");
    }
    direct_move();
    reverse_move(X);
    free(gauss_A);
    free(gauss_B);
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
        temp = gauss_A[N * i + i];
        for (j = i; j < N; j++) {
            gauss_A[N * i + j] /= temp;
        }
        gauss_B[i] /= temp;
        pthread_barrier_wait(&bar1);
        pthread_barrier_wait(&bar2);
    }
}

/*
 * Выполнение обратного хода Гаусса
 * 1*X[n] = B[n]                            | 1  a22 | * | x1 | = | b1 |
 * 1*X[n-1] + a[n-1][n]*X[n] = B[n-1]       | 0   1  | * | x2 | = | b2 |
 * X[n-1] = B[n-1] - X[n]*a[n-1][n]*X[n]
 * ...
 */
void reverse_move(double *X) {
    // double *X - вектор неизвестных
    int i = 0, j = 0;
    for (i = N - 1; i >= 0; i--) {
        X[i] = gauss_B[i];
        for (j = N - 1; j > i; j--) {
            X[i] -= X[j] * gauss_A[i * N + j];
        }
    }
}

/*
 * Копирование матрицы A и B во внутренние массивы
 */
void matrix_initialization(const double *A, const double *B) {
    int i, j;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            gauss_A[N * i + j] = A[N * i + j];;
            gauss_B[i] = B[i];
        }
    }
}

/*
 * Функция которую выполняют потоки для исключения элементов
 * матрицы свободных коэффициентов по столбцам.
 * Распараллеливание ведется следующим образом:
 *
 *  thread1| 1 a11 a12 | b1 |
 *  thread2| 0 a21 a22 | b2 |
 *  thread1| 0 a31 a32 | b3 |
 */
void *thread_exclusion(void *threads) {
    double temp;
    Threads *thread = (Threads *) threads;
    for (int j = 0; j < N; j++) {
        if (j >= N - 1) // условие остановки потоков
            break;
        pthread_barrier_wait(&bar1); //поток ожидает, когда main закончит работу с ведущей строкой

        for (int i = j + 1 + thread->start; i < N; i += thread->step) { // определение строки, которую обрабатывает поток
            temp = gauss_A[N * i + j];
            if (temp == 0) // если элемент уже нулевой,
                continue;  // то не производить исключение
            gauss_B[i] = (gauss_B[i] / temp) - gauss_B[j];
            for (int k = j; k < N; k++) {
                gauss_A[N * i + k] /= temp;
                gauss_A[N * i + k] -= gauss_A[N * j + k];
            }
        }
        pthread_barrier_wait(&bar2); // поток закончил обрабатывать столбец
        // поток main начинает работать со следующей ведущей строкой
    }
    pthread_barrier_wait(&bar1);
    pthread_barrier_wait(&bar2);
    pthread_exit(0);
}
