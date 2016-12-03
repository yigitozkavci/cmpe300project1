#include "convolution.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

double** get_smoother_conv() {
  double** smoother_conv = (double**)malloc(sizeof(double*) * CONVOLUTION_SIZE);
  for(int i = 0; i < CONVOLUTION_SIZE; i++) {
    *(smoother_conv + i) = (double*)malloc(sizeof(double) * CONVOLUTION_SIZE);
  }
  for(int i = 0; i < CONVOLUTION_SIZE; i++) {
    for(int j = 0; j < CONVOLUTION_SIZE; j++) {
      *(*(smoother_conv + i) + j) = 1.0/9;
    }
  }
  return smoother_conv;
}

void convolute_i(int** arr, int arr_size, int** convoluter) {
  for(int i = 1; i < arr_size - 1; i++) {
    for(int j = 1; j < arr_size - 1; j++) {
      convolute_point_i(i, j, arr, convoluter);
    }
  }
}

void convolute_point_i(int x, int y, int** arr, int** convoluter) {
  double total = 0;
  int convoluter_x = 0;
  int convoluter_y = 0;
  for(int i = x-1; i <= x+1; i++, convoluter_x++) {
    for(int j = y-1; j <= y+1; j++, convoluter_y++) {
      int arr_val = *(*(arr + i) + j);
      int convoluter_val = *(*(convoluter + convoluter_x) + convoluter_y);
      total += arr_val * convoluter_val;
    }
    convoluter_y = 0;
  }
  *(*(arr + x) + y) = total;
}

int** convolute_d(int** arr, int arr_size, double** convoluter) {
  int** convoluted_matrix = (int**)malloc(sizeof(int*) * (arr_size - 1));
  for(int i = 0; i < arr_size - 1; i++) {
    *(convoluted_matrix + i) = (int*)malloc(sizeof(int) * (arr_size - 1));
  }

  for(int i = 1; i < arr_size - 1; i++) {
    for(int j = 1; j < arr_size - 1; j++) {
      convolute_point_d(i, j, arr, convoluter, convoluted_matrix);
    }
  }

  return convoluted_matrix;
}

void convolute_point_d(int x, int y, int** arr, double** convoluter, int** convoluted_matrix) {
  double total = 0;
  int convoluter_x = 0;
  int convoluter_y = 0;
  for(int i = x-1; i <= x+1; i++, convoluter_x++) {
    for(int j = y-1; j <= y+1; j++, convoluter_y++) {
      int arr_val = *(*(arr + j) + i);
      double convoluter_val = *(*(convoluter + convoluter_y) + convoluter_x);
      total += arr_val * convoluter_val;
    }
    convoluter_y = 0;
  }
  int result;
  if(fabs(total - round(total)) < 0.001) {
    result = (int) round(total);
  } else {
    result = (int) total;
  }
  *(*(convoluted_matrix + y - 1) + x - 1) = result;
}
