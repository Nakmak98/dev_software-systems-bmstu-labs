/*
 * Программа для расчёта уравнения струны методом конечных разностей (FDM).
 * Задание внешней силы не учтено. Начальное ускорение равно 0.
 * Получается уравнение свободных колебаний струны.
 *
 * Компиляция: mpicc string.c -o string -lm
 * Запуск: mpiexec -n (кол-во процессов кратное 8) ./string <количество узлов кратное 8> <время>
 * Пример запуска: mpiexec -n 4 ./string 80 100
 * Преждевременное завершение: ctrl+c
 *
 * Краткое описание работы процессов:
 *       Процесс main (pid = 0):
 *       Цикл по временным слоям:
 *          - Распределение по процессам объема вычислений
 *          - MPI_Scatterv()
 *          - Вывод графика (gnuplot)
 *          - очистка ненужного временного слоя
 *       Служебные процессы (pid > 0):
 *          - Расчет нового временного слоя
 *          - Отправка в main MPI_Gather()
 *
 *  j = 2 |0 x - -|x - - -|x - - 0| // пример разностного шаблона
 *  j = 1 |0 + + +|+ + - +|+ + - 0| // x - искомый узел
 *  j = 0 |0 + - -|+ - - -|+ - - 0| // + - необходимые узлы для расчета
 *         0 1 2 3 .......      n-1
 * */
#include "mpi_string.h"

int main(int argc, char *argv[]) {
    int pid;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_process);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    FILE *gp = popen("gnuplot -persist", "w");
    int root = 0;
    input(argc, argv);
    time_intervals = time / DT;
    const int N = number_of_nodes * 3; // 2 временных слоя для рассчета + 1 для вычисленных узлов
    double *U = (double *) malloc(N * sizeof(double));
    double *u = U;
    double *swap_temp;
    // блок объявлений структур данных для MPI_Scatterv и MPI_Gather
    calculated_nodes = number_of_nodes / number_of_process;
    int sendcounts[number_of_process];
    int displacment[number_of_process];
    double *scatter_sendbuf = (double *) malloc(number_of_nodes * sizeof(double));
    double *scatter_recvbuf = (double *) malloc((calculated_nodes + 2) * 2 * sizeof(double));
    double *gather_sendbuf = (double *) malloc(calculated_nodes * sizeof(double));

    if (pid == root)
        initial_values(u, N);
    for (int j = 0; j < time_intervals - 1; j++) {
        scatter_nodes_to_process(u, scatter_sendbuf, scatter_recvbuf,
                                 sendcounts, displacment, pid);
        FDM(scatter_recvbuf, gather_sendbuf, pid);
        MPI_Gather(gather_sendbuf, calculated_nodes, MPI_DOUBLE, u += (number_of_nodes * 2), calculated_nodes,
                   MPI_DOUBLE, 0,
                   MPI_COMM_WORLD);
        if (pid == root) {
            create_plot(u, gp, 0);
            // сохранение текущего и вычисленного временного слоя
            u -= number_of_nodes;
            swap_temp = U;
            swap_rows(u, swap_temp);
            u = U;
        }
    }
    pclose(gp);
    free(scatter_sendbuf);
    free(scatter_recvbuf);
    free(gather_sendbuf);
    free(U);
    MPI_Finalize();
    return 0;
}

// Проверка аргументов, инициализация расчетных значений
void input(int argc, char **argv) {
    if (argc < 2) {
        printf("Not enough arguments!\n");
        printf("There must be:\nnumber of threads\n");
        printf("number of nodes\nmodelling time\n");
        exit(1);
    }

    if ((number_of_nodes = atof(argv[1])) % 8) {
        printf("Invalid number of nodes\n");
        exit(2);
    }

    if ((time = atof(argv[2])) <= 0) {
        printf("Invalid modelling time\n");
        exit(3);
    }

}

// Задание начального положения струны
void initial_values(double *A, int n) {
    clear_arr(A, n);
    double h = 0.1;
    double x = -1;
    for (int i = 1; i < number_of_nodes - 1; i++) {
        x = x + h;
        A[number_of_nodes + i] = 2 * cos(x) + sin(x / 2);
    }
}

void clear_arr(double *A, int n) {
    for (int i = 0; i < n; i++) {
        A[i] = 0;
    }
}


// распределение узлов по процессам
void scatter_nodes_to_process(const double *nodes, double *sendbuf, double *recvbuf,
                              int *sendcounts, int *displacment, int pid) {
    /*
     * sendbuf, sendcounts заполняется для MPI_Scatterv()
     * Крайним узлам нужно меньше узлов для вычислений, т.к. на них граничные условия.
     * Промежуточным процессам необходимы крайние узлы каждого процесса.
     */
    double *start = recvbuf;
    if (pid == 0) {
        int n = 0;
        for (int process = 0; process < number_of_process; process++) {
            displacment[process] = process == 0 ? 0 : displacment[process - 1] - 2 + n;
            n = ((process == 0) || (process == number_of_process)) ?
                calculated_nodes + 1 : calculated_nodes + 2;
            sendcounts[process] = n;
        }
    }

    // j < 2 т.к. для вычисления нового временного слоя необходимо 2 предыдущих
    for (int j = 0; j < 2; j++, recvbuf += calculated_nodes + 2) {
        if (pid == 0) {
            for (int i = 0; i < number_of_nodes; i++)
                sendbuf[i] = nodes[i];
            nodes += number_of_nodes;
        }
        MPI_Scatterv(sendbuf, sendcounts, displacment,
                     MPI_DOUBLE, recvbuf, calculated_nodes + 2,
                     MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }
    recvbuf = start;
}

// Итерация метода конечных разностей
void FDM(const double *in_nodes, double *out_nodes, int pid) {
    int n = calculated_nodes + 2; // максимальное количество расчетных узлов
    int out_nodes_len = calculated_nodes;
    int j = 1;  // расчет на 2-x временных слоях: j, j-1

    int i = 0;
    const double a = 1.0; // физический параметр, зависящий от натяжения и материала струны

    // учет граничных условий
    if (pid == 0) {
        out_nodes[0] = 0;
        i = 1;
    } else if (pid == number_of_process - 1) {
        out_nodes[out_nodes_len] = 0;
        out_nodes_len--;
    }
    for (int k = 1; i < out_nodes_len; i++, k++) {
        out_nodes[i] = (a * a) * (in_nodes[j * n + (k + 1)] - 2 * in_nodes[j * n + k] + in_nodes[j * n + (k - 1)])
                       * ((DT / DX) * (DT / DX)) + 2 * in_nodes[j * n + k] -
                       in_nodes[(j - 1) * n + k];
    }
}

// вывод графика с помощью утилиты gnuplot
void create_plot(double *A, FILE *gp, int j) {
    fprintf(gp, "set xrange [0:%d]\n"
                "set yrange [-80:80]\n", number_of_nodes);
    fprintf(gp, "plot '-' with lines linestyle 1\n");
    for (int i = 0; i < number_of_nodes; i++) {
        fprintf(gp, " %-15d %# -15g\n", i, A[j * number_of_nodes + i]);
    }
    fprintf(gp, "e\n");
    fprintf(gp, "pause 0.1\n"); // скорость анимации
}

// Замена старых услов на новые
void swap_rows(double *p, double *t) {
    // *p указывает на j = 1, *t на j = 0
    for (int k = 0; k < 2; ++k)
        for (int i = 0; i < number_of_nodes; ++i)
            *t++ = *p++;
}

