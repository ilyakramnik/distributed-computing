#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <mpi.h>
#include <time.h>

#include "matxvec.h"


int32_t comm_size = -1, rank = -1;

void input_vector(double_t* v, uint32_t n)
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

void print_usage(char *bin_name)
{
  fprintf(stderr, "Usage: mpiexec -n <copies> %s <rows> <cols>\n\
                \nRequired arguments:\
                \n  'copies' - number of parallel processes\
                \n  'rows'   - number of rows\
                \n  'cols'   - number of columns\n", bin_name);
}

#ifdef TEST
void _row_div_time_measurement_test(uint32_t rows_min, uint32_t rows_max, uint32_t rows_step_size, uint32_t cols_min, uint32_t cols_max, uint32_t cols_step_size)
{
  double_t time_start, time_end;
  double_t* vec = NULL;
  double_t* out_vec = NULL;
  double_t* matrix = NULL;
  double_t* local_res = NULL;

  if (rank == 0)
    printf("Split by rows:\n");

  for (uint32_t rows = rows_min; rows <= rows_max; rows += rows_step_size) {
    for (uint32_t cols = cols_min; cols <= cols_max; cols += cols_step_size) {
      vec = malloc(cols * sizeof(double_t));
      input_vector(vec, cols);

      matrix = malloc(rows * cols * sizeof(double_t*));
      input_vector(matrix, cols * rows);

      if (rank == 0) {
        out_vec = malloc(rows * sizeof(double_t));
        memset(out_vec, 0, rows * sizeof(double_t));
      }

      uint32_t local_rows = rows / comm_size;

      local_res = malloc(local_rows * sizeof(double_t));
      memset(local_res, 0, local_rows * sizeof(double_t));

      time_start = MPI_Wtime();

      for (uint32_t row = 0; row < local_rows; row++)
      {
        double_t* row_ptr = matrix + row * cols;
        double_t res = 0;
        for (uint32_t i = 0; i < cols; i++) 
          res += (row_ptr[i] * vec[i]);
        local_res[row] = res;
      }

      MPI_Reduce(local_res, out_vec, local_rows, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
      time_end   = MPI_Wtime();

      if (rank == comm_size - 1)
        printf("%d %6dx%6d %f\n", comm_size, rows, cols, time_end - time_start);

      free(local_res);
      free(matrix);
      free(vec);
      if (rank == 0)
        free(out_vec);
    }
  }
}

void _col_div_time_measurement_test(uint32_t rows_min, uint32_t rows_max, uint32_t rows_step_size, uint32_t cols_min, uint32_t cols_max, uint32_t cols_step_size)
{
  double_t time_start, time_end;
  double_t* vec = NULL;
  double_t* out_vec = NULL;
  double_t* matrix = NULL;
  double_t* local_res = NULL;

  if (rank == 0)
    printf("Split by column:\n");

  for (uint32_t cols = cols_min; cols <= cols_max; cols += cols_step_size) {
    for (uint32_t rows = rows_min; rows <= rows_max; rows += rows_step_size) {
      vec = malloc(cols * sizeof(double_t));
      input_vector(vec, cols);

      local_res = malloc(rows * sizeof(double_t));
      memset(local_res, 0, rows * sizeof(double_t));

      matrix = malloc(cols * rows * sizeof(double_t*));
      input_vector(matrix, cols * rows);

      if (rank == 0) {
        out_vec = malloc(rows * sizeof(double_t));
        memset(out_vec, 0, rows * sizeof(double_t));
      }

      MPI_Barrier(MPI_COMM_WORLD);

      time_start = MPI_Wtime();

      uint32_t local_cols = cols / comm_size;
      uint32_t col_start = rank * local_cols;
      if (rank + 1 == comm_size)
        local_cols += cols % comm_size;
      uint32_t col_end = col_start + local_cols;

      for (uint32_t row = 0; row < rows; row++) {
        for (uint32_t col = col_start; col < col_end; col++) {
          local_res[row] += matrix[row * cols + col] * vec[col];
        }
      }

      MPI_Reduce(local_res, out_vec, rows, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

      //mul_mat_by_vec(Column, matrix, vec, out_vec, rows, cols, rank, comm_size);
      time_end   = MPI_Wtime();

      if (rank == comm_size - 1)
        printf("%d %6dx%6d %f\n", comm_size, rows, cols, time_end - time_start);

      free(matrix);
      free(vec);
      if (rank == 0)
        free(out_vec);
    }
  }
}

void _block_div_time_measurement_test(uint32_t rows_min, uint32_t rows_max, uint32_t rows_step_size, uint32_t cols_min, uint32_t cols_max, uint32_t cols_step_size)
{
  double_t time_start, time_end;
  double_t* vec = NULL;
  double_t* out_vec = NULL;
  double_t* matrix = NULL;
  double_t* local_res = NULL;

  if (rank == 0)
    printf("Split by block:\n");

  for (uint32_t rows = rows_min; rows <= rows_max; rows += rows_step_size)
  {
    for (uint32_t cols = cols_max; cols >= cols_min; cols -= cols_step_size)
    {
      vec = malloc(cols * sizeof(double_t));
      input_vector(vec, cols);

      local_res = malloc(rows * sizeof(double_t));
      memset(local_res, 0, rows * sizeof(double_t));

      matrix = malloc(rows * cols * sizeof(double_t*));
      input_vector(matrix, rows * cols);

      if (rank == 0) {
        out_vec = malloc(rows * sizeof(double_t));
        memset(out_vec, 0, rows * sizeof(double_t));
      }

      MPI_Barrier(MPI_COMM_WORLD);

      uint32_t rows_half = rows / 2;
      uint32_t cols_half = cols / 2;
      /* 0 ... rows_half - 1 rows_half ... rows - 1 */
      /* 0 ... cols_half - 1 cols_half ... cols - 1 */
      uint32_t row_start = 0, row_end = rows_half, offset = 0;

      switch (rank) {
        case 0:
          offset = cols_half;
          break;
        case 1:
          break;
        case 2:
          row_start = rows_half;
          row_end = rows;
          break;
        case 3:
          row_start = rows_half;
          row_end = rows;
          offset = cols_half;
          break;
      }

      time_start = MPI_Wtime();

      for (uint32_t row = row_start; row < row_end; row++)
      {
        double_t* row_ptr = matrix + row * cols + offset;
        double_t* col_ptr = vec + offset;
        double_t res = 0;
        for (uint32_t i = 0; i < cols_half; i++) 
          res += (row_ptr[i] * col_ptr[i]);
        local_res[row] = res;
      }

      MPI_Reduce(local_res, out_vec, rows, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

      //mul_mat_by_vec(Block, matrix, vec, out_vec, rows, cols, rank, comm_size);
      time_end   = MPI_Wtime();

      if (rank == comm_size - 1)
        printf("%d %5dx%5d %f\n", comm_size, rows, cols, time_end - time_start);

      free(matrix);
      free(local_res);
      free(vec);
      if (rank == 0)
        free(out_vec);
    }
  }
}

void time_measurement_test()
{
  _row_div_time_measurement_test(100000, 100000, 10000, 2000, 2000, 1);
  _col_div_time_measurement_test(2000, 2000, 1, 70000, 100000, 10000);
  _row_div_time_measurement_test(2000, 2000, 1, 90000, 100000, 10000);
  _col_div_time_measurement_test(70000, 100000, 10000, 2000, 2000, 1);
  if (comm_size == 4) {
    _block_div_time_measurement_test(20000, 100000, 10000, 2000, 2000, 1);
    _block_div_time_measurement_test(2000, 2000, 1, 20000, 100000, 10000);
  }
}
#endif

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#ifndef TEST
  double_t* vec = NULL;
  double_t* out_vec = NULL;
  double_t* matrix = NULL;

  uint32_t rows, cols; /* mat(rows x cols) x vec(cols x 1) = res(rows x 1) */

  if (argc < 3) {
    if (rank == 0) print_usage(argv[0]);

    goto finalize;
  }

  if (rank == 0)
    srand(time(NULL));

  /************************/
  /*** Data preparation ***/
  /************************/
  rows = atoi(argv[1]);
  cols = atoi(argv[2]);

  if (comm_size > rows) {
    if (rank == 0)
      fprintf(stderr, "[Error] Number of processes should be less than number of rows\n");
    goto finalize;
  }

  vec = malloc(cols * sizeof(double_t));
  input_vector(vec, cols);

  matrix = malloc(rows * cols * sizeof(double_t));
  input_vector(matrix, cols * rows);

  if (rank == 0) {
    out_vec = malloc(rows * sizeof(double_t));
    memset(out_vec, 0, rows * sizeof(double_t));
  }

  if (rank == 0) {
    printf("\nMatrix (%dx%d):\n", rows, cols);
    for (uint32_t row = 0; row < rows; row++) {
      for (uint32_t col = 0; col < cols; col++) {
        printf("%.2lf ", matrix[row * cols + col]);
      }
      putc('\n', stdout);
    }
    printf("Vector (%dx1):\n", cols);
    for (uint32_t col = 0; col < cols; col++) {
      printf("%.2lf ", vec[col]);
    }
    putc('\n', stdout);
  }
  /************************/
  MPI_Barrier(MPI_COMM_WORLD);

  mul_mat_by_vec(Row, matrix, vec, out_vec, rows, cols, rank, comm_size);
  // mul_mat_by_vec(Column, matrix, vec, out_vec, rows, cols, rank, comm_size);
  // mul_mat_by_vec(Block, matrix, vec, out_vec, rows, cols, rank, comm_size);

  if (rank == 0) {
    printf("\nResult vector (%dx1):\n", rows);
    for (uint32_t row = 0; row < rows; row++) {
      printf("%.4lf ", out_vec[row]);
    }
  }

  /**************************/
  /*** Clean and finilize ***/
  /**************************/
  free(matrix);
  free(vec);
  if (rank == 0)
    free(out_vec);

finalize:
#else
  time_measurement_test();
#endif /* ifndef TEST */

  MPI_Finalize();
  /**************************/

  return EXIT_SUCCESS;
}
