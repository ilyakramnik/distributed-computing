#include "cannon.h"
#include <mpi.h>
#include <stdlib.h>

void _cannon_mat_n_mat(double_t* const first_m, double_t* const second_m, double_t* out_m, uint32_t N)
{
  for (uint32_t i = 0; i < N; i++) {
    for (uint32_t j = 0; j < N; j++) {
      for (uint32_t k = 0; k < N; k++) {
        out_m[i * N + j] += first_m[i * N + k] * second_m[k * N + j];
      }
    }
  }
}

void cannon_method(double_t* const first_matrix, double_t* const second_matrix, double_t* out_matrix, uint32_t n, int32_t rank, int32_t comm_size)
{
  uint32_t n_local, n_local_2;

  double_t* first_matrix_local;
  double_t* second_matrix_local;
  double_t* out_matrix_local;

  /************************/
  /*** Data preparation ***/
  /************************/
  n_local   = n / sqrt(comm_size); /* calculate size for matrix for every proc */
  n_local_2 = n_local * n_local;

  first_matrix_local  = malloc(n_local_2 * sizeof(double_t*));
  second_matrix_local = malloc(n_local_2 * sizeof(double_t*));
  out_matrix_local    = malloc(n_local_2 * sizeof(double_t*));

  /*************************/
  /****** Calculation ******/
  /*************************/

  MPI_Scatter(first_matrix,  n_local_2, MPI_DOUBLE, first_matrix_local,  n_local_2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Scatter(second_matrix, n_local_2, MPI_DOUBLE, second_matrix_local, n_local_2, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  _cannon_mat_n_mat(first_matrix_local, second_matrix_local, out_matrix_local, n_local);

  MPI_Gather(out_matrix_local, n_local_2, MPI_DOUBLE, out_matrix, n_local_2, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  /********************/
  /*** Clean memory ***/
  /********************/
  free(out_matrix_local);
  free(second_matrix_local);
  free(first_matrix_local);
}
