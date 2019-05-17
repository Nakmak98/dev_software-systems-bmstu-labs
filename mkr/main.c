#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi/mpi.h>
#include <math.h>

#define DX 1
#define DT 1

int number_of_nodes, time_intervals, nodes_for_calculation, calculated_nodes;
double time;

void input(int, char **);

double initial_position(double);

void initial_and_boundary_values(double *, int);

void FDM(double *, double *, int);

void create_plot(double *, FILE *);

void print_matrix(double *, int, int);

void get_nodes_per_process(const double *nodes, double *per_process, int time_interval);

void input(int argc, char **argv) {
    if (argc < 2) {
        printf("Not enough arguments!\n");
        printf("There must be:\nnumber of threads\n");
        printf("number of nodes\nmodelling time\n");
        exit(1);
    }

    if ((number_of_nodes = atof(argv[1])) < 3) {
        printf("Invalid number of nodes\n");
        exit(2);
    }

    if ((time = atof(argv[2])) <= 0) {
        printf("Invalid modelling time\n");
        exit(3);
    }

}

void initial_and_boundary_values(double *A, int n) {
//    printf("nitialization\n");
    for (int i = 0; i < n; i++) {
        A[i] = 0;
    }

    //boundary conditions (may be not null)
    A[0] = 0;
    A[n - 1] = 0;

    //initial conditions
    double h = 16.0 / number_of_nodes;
    double x = -8;
    for (int i = 1; i < number_of_nodes - 1; i++) {
        x = x + h;
//        printf("%d) x = %f\n",i, initial_position(x));
        A[number_of_nodes + i] = initial_position(x);
    }
}

double initial_position(double x) {
    return cos(x);
}

void FDM(double *z, double *g, int pid) {
    // TODO: Приложить внешнюю силу
    int n = nodes_for_calculation / 2;
    int l = calculated_nodes;
    int j = 1;
    double temp = 0;
    double a = 1.0;
    double t[l];
    for (int m = 0; m < l; ++m) {
        g[m] = 0;
        t[m] = g[m];
    }
    for (int k = 1, i=0; i < l; i++, k++) {
        double z0 = z[j * n + (k - 1)];
        double z1 = z[j * n + k];
        double z2 = z[j * n + (k + 1)];
        temp = (a * a) * (z[j * n + (k + 1)] - 2 * z[j * n + k] + z[j * n + (k - 1)])
               * ((DT / DX) * (DT / DX)) + 2 * z[j * n + k] -
               z[(j - 1) * n + k];
        g[i] = temp;
    }
//    for (int m = 0; m < l; ++m) {
//        printf("pid = %d g[%d] = %f\n",pid, m, g[m]);
//    }

}

void create_plot(double A[], FILE *gp) {
    for (int j = 1; j < time_intervals; j++) {
        fprintf(gp, "set xrange [0:%d]\n"
                    "set yrange [-80:80]\n", number_of_nodes);
        fprintf(gp, "plot '-' with lines linestyle 1\n");
        for (int i = 0; i < number_of_nodes; i++) {
            fprintf(gp, " %-15d %# -15g\n", i, A[j * number_of_nodes + i]);
        }
        fprintf(gp, "e\n");
        fprintf(gp, "pause 0.1\n");
        fflush(gp);
    }
}

void get_nodes_per_process(const double *nodes, double *per_process, int time_interval) {
    /*
     * Функция вычисляет узлы, которые достанутся конкретным процессам
     * Для t-1  (t == 0) момента времени используются фиктивные узлы равные начальной скорости струны
     * (полагаем, что она равна 0)
     * для моментов времени t-1 (t != 0) используются узлы на предыдущих итерациях
     *
     *  u заполняется для удобного разбиения функцией MPI_Scatter на массивы
     * для каждого процесса
     * */
    int f = number_of_nodes * (time_intervals + 1);
    double p[f];
//    printf("PIZDEC\n");
    int r = 0;


    int k = 0;
    int m = number_of_nodes;
    int n = nodes_for_calculation / 2;
    for (int l = 0; l < 2; l++, nodes += 3)
        for (int j = 0; j < 2; j++)
            for (int i = 0; i < n; i++, k++)
                per_process[k] = nodes[j * m + i];
}

void print_matrix(double *matrix, int t, int n) {
    int k = 0;
    for (int i = 0; i < t; i++) {
        for (int j = 0; j < n; j++, k++) {
            if (k == number_of_nodes){
                k = 0;
                putchar('\n');
            }

            printf("%d) %f\n",k, matrix[j * t + i]);
        }

    }
    putchar('\n');
}

int main(int argc, char *argv[]) {
    int number_of_process, pid;
    FILE *gp = popen("gnuplot -persist", "w");
    MPI_Init(&argc, &argv);
    /*
     * Процесс main (pid = 0):
     *  1) Распределение по процессам объем вычислений
     *     nodes_for_calculation = number_of_nodes / number_of_process
     *  2) Цикл по временному слою
     *     MPI_Scatter(p, nodes_for_calculation, MPI_DOUBLE, a, nodes_for_calculation, 0, MPI_COMM_WORLD)
     *  3) Вывод графика
     *
     * Служебные процессы (pid > 0):
     *  1) Расчет выделенных узлов и запись в массив a
     *  2) Отправка в main
     *     MPI_Gather(a, nodes_for_calculation, MPI_DOUBLE, p+=(p+nodes_for_calculation), nodes_for_calculation, MPI_DOUBLE, 0, MPI_COMM_WORLD)
     * */
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_process);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    input(argc, argv);
    time_intervals = (int) (time / DT); // WARNING: возможны проблемы с дискретизацией по времени
    nodes_for_calculation = 2 * (number_of_nodes / number_of_process) + 2;
    const int N = number_of_nodes * (time_intervals + 1);
    double U[N];
    double *p = U;
    int root = 0;
    calculated_nodes = (number_of_nodes - 2) / number_of_process;
    if (pid == root)
        initial_and_boundary_values(p, N);

    double *u = (double *) malloc(number_of_process * nodes_for_calculation*sizeof(double));
    double *z = (double *) malloc(nodes_for_calculation * sizeof(double));
    double *g = (double *) malloc(calculated_nodes * sizeof(double));
    for (int j = 0; j < time_intervals - 1; j++) {
        if (pid == root)
            get_nodes_per_process(p, u, j);
        MPI_Scatter(u, nodes_for_calculation, MPI_DOUBLE,
                    z, nodes_for_calculation, MPI_DOUBLE, 0, MPI_COMM_WORLD);;
        FDM(z, g, pid);
        MPI_Barrier(MPI_COMM_WORLD);
        if (pid == root)
            p += (number_of_nodes * 2) + 1;
        MPI_Gather(g, calculated_nodes, MPI_DOUBLE, p, calculated_nodes, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        if (pid == root)
            p -= number_of_nodes + 1;
        MPI_Barrier(MPI_COMM_WORLD);
    }
    if (pid == root){
        create_plot(U, gp);
//        print_matrix(U, 1, N);
    }
    free(u);
    free(z);
    free(g);
    MPI_Finalize();
}