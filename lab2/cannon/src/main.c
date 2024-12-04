#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <mpi.h>
#include <time.h>

#include "cannon.h"


int32_t comm_size = -1, rank = -1;

void input_vector_row_div(double_t* v, uint32_t n)
{
  if (rank == 0)
  {
    for (uint32_t i = 0 ; i < n ; i++)
    {
      v[i] = (double_t)(rand() % 100) / 7;
    }
  }
  MPI_Bcast(v, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}

void print_1d_matrix(const char* matrix_name, double_t* matrix, uint32_t n)
{
  printf("\n%s (%dx%d):", matrix_name, n, n);
  for (uint32_t i = 0; i < n * n; i++) {
    if (i % n == 0)
      putc('\n', stdout);
    printf("%4.2lf ", matrix[i]);
  }
  putc('\n', stdout);
}

void print_usage(char *bin_name)
{
  fprintf(stderr, "Usage: mpiexec -n <copies> %s <N>\n\
                \nRequired arguments:\
                \n  'copies' - number of parallel processes\
                \n  'N'      - matrix size NxN\n", bin_name);
}

#ifdef TEST
void time_measurement_test()
{
  double_t time_start, time_end;
  double_t* first_matrix;
  double_t* second_matrix;
  double_t* out_matrix;
  uint32_t n;

  for (uint32_t i = 50; i <= 2800; i += 50)
  {
    n = i;

    first_matrix  = malloc(n * n * sizeof(double_t*));
    second_matrix = malloc(n * n * sizeof(double_t*));
    out_matrix    = malloc(n * n * sizeof(double_t*));

    for (uint32_t row = 0; row < n; row++) {
      input_vector_row_div(first_matrix  + (n * row), n);
      input_vector_row_div(second_matrix + (n * row), n);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    time_start = MPI_Wtime();
    cannon_method(first_matrix, second_matrix, out_matrix, n, rank, comm_size);
    time_end   = MPI_Wtime();

    if (rank == 0)
      printf("%d %4d %f\n", comm_size, n, time_end - time_start);

    free(out_matrix);
    free(second_matrix);
    free(first_matrix);
  }
}
#endif


int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#ifndef TEST
  double_t* first_matrix  = NULL;
  double_t* second_matrix = NULL;
  double_t* out_matrix    = NULL;

  uint32_t n, n_2; /* mat(n x n) x mat(n x n) = mat(n x n) */

  if (argc < 2) {
    if (rank == 0) print_usage(argv[0]);

    goto finalize;
  }

  n   = atoi(argv[1]);
  n_2 = n*n;

  if (rank == 0)
    srand(time(NULL));

  /************************/
  /*** Data preparation ***/
  /************************/

  first_matrix  = malloc(n_2 * sizeof(double_t*));
  second_matrix = malloc(n_2 * sizeof(double_t*));
  out_matrix    = malloc(n_2 * sizeof(double_t*));

  for (uint32_t row = 0; row < n; row++) {
    input_vector_row_div(first_matrix + (n * row), n);
    input_vector_row_div(second_matrix + (n * row), n);
  }
  memset(out_matrix, 0, n * n * sizeof(double_t*));

  if (rank == 0) {
    print_1d_matrix("Matrix 1", first_matrix, n);
    print_1d_matrix("Matrix 2", second_matrix, n);
  }
  /************************/
  MPI_Barrier(MPI_COMM_WORLD);

  cannon_method(first_matrix, second_matrix, out_matrix, n, rank, comm_size);

  if (rank == 0)
    print_1d_matrix("Output Matrix", out_matrix, n);

  /**************************/
  /*** Clean and finilize ***/
  /**************************/
  free(first_matrix);
  free(second_matrix);
  free(out_matrix);

finalize:
#else

  time_measurement_test();
#endif

  MPI_Finalize();
  /**************************/

  return EXIT_SUCCESS;
}
