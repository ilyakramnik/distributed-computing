#include "matxvec.h"

#include <mpi.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


inline double_t _sum_vec(double_t* const vector, uint32_t n)
{
  double_t res = 0;

  for (uint32_t i = 0; i < n; i++)
    res += vector[i];

  return res;
}

inline void _mul_col_by_const(double_t* const matrix, double_t* out, double_t* vector, uint32_t rows, uint32_t cols, int32_t rank, int32_t comm_size)
{
  uint32_t local_cols = cols / comm_size;
  uint32_t col_start = rank * local_cols;
  if (rank + 1 == comm_size)
    local_cols += cols % comm_size;
  uint32_t col_end = col_start + local_cols;

  for (uint32_t row = 0; row < rows; row++) {
    for (uint32_t col = col_start; col < col_end; col++) {
      out[row] += matrix[row * cols + col] * vector[col];
    }
  }
}

inline double_t _mul_row_by_col(double_t* const row, const double_t* const col, uint32_t n)
{
  double_t res = 0;
  for (uint32_t i = 0; i < n; i++) 
    res += (row[i] * col[i]);
  return res;
}

inline void _row_split_mul(double_t* const matrix, const double_t* const vector, double_t* out, uint32_t rows, uint32_t cols, int32_t rank, int32_t comm_size)
{
  uint32_t local_rows = rows / comm_size;

  double_t* local_res = malloc(local_rows * sizeof(double_t));
  memset(local_res, 0, local_rows * sizeof(double_t));

  for (uint32_t row = 0; row < local_rows; row++)
  {
    local_res[row] = _mul_row_by_col(matrix + row * cols, vector, cols);
  }
  //MPI_Gather(local_res, local_rows, MPI_DOUBLE, out, local_rows, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Reduce(local_res, out, local_rows, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  /* cleanup */
  free(local_res);
}


inline void _column_split_mul(double_t* const matrix, double_t* const vector, double_t* out, uint32_t rows, uint32_t cols, int32_t rank, int32_t comm_size)
{
  double_t* res_vec = malloc(rows * sizeof(double_t));
  memset(res_vec, 0, rows * sizeof(double_t));

  _mul_col_by_const(matrix, res_vec, vector, rows, cols, rank, comm_size);

  MPI_Reduce(res_vec, out, rows, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  /* cleanup */
  free(res_vec);
}

inline void _block_split_mul(double_t* const matrix, double_t* const vector, double_t* out, uint32_t rows, uint32_t cols, int32_t rank)
{
  uint32_t rows_half = rows / 2;
  uint32_t cols_half = cols / 2;
  /* 0 ... rows_half - 1 rows_half ... rows - 1 */
  /* 0 ... cols_half - 1 cols_half ... cols - 1 */
  double_t* cur_out = malloc(rows * sizeof(double_t));
  memset(cur_out, 0, rows * sizeof(double_t));

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

  for (uint32_t row = row_start; row < row_end; row++)
    cur_out[row] += _mul_row_by_col((matrix + row * cols) + offset, vector + offset, cols_half);

  MPI_Reduce(cur_out, out, rows, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  free(cur_out);
}

void mul_mat_by_vec(enum split method, double_t* const matrix, double_t* const vector, double_t* out, uint32_t rows, uint32_t cols, int32_t rank, int32_t comm_size)
{
  switch (method)
  {
  case Row:
    _row_split_mul(matrix, vector, out, rows, cols, rank, comm_size);
    break;
  case Column:
    _column_split_mul(matrix, vector, out, rows, cols, rank, comm_size);
    break;
  case Block:
    if (comm_size == 4)
      _block_split_mul(matrix, vector, out, rows, cols, rank);
    break;
  }
}

