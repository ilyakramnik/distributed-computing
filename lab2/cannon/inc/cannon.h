#pragma once

#include <math.h>
#include <stdint.h>

void cannon_method(double_t* const first_matrix, double_t* const second_matrix, double_t* out_matrix, uint32_t n, int32_t rank, int32_t comm_size);
