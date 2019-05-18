#ifndef MKR_MPI_STRING_H
#define MKR_MPI_STRING_H

#include <stdio.h>
#include <stdlib.h>
#include <mpi/mpi.h>
#include <math.h>

#define DX 1
#define DT 1

int number_of_nodes, number_of_process;
int time_intervals, time;
int calculated_nodes;

void input(int, char **);

void clear_arr(double*, int);

void initial_values(double*, int);

void scatter_nodes_to_process(const double *, double *, double *, int *, int *, int);

void FDM(const double *, double *, int);

void create_plot(double *, FILE *, int);

void swap_rows(double *, double *);

#endif //MKR_MPI_STRING_H
