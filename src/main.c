#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "convolution.h"

int main() {
  printf("Hello!\n");
  int size = 5;
  int** example_arr = (int**)malloc(sizeof(int*) * size);
  for(int i = 0; i < size; i++) {
    *(example_arr + i) = (int*)malloc(sizeof(int) * size);
  }

  FILE* file = fopen("input2.txt", "r");
  for(int i = 0; i < size; i++) {
    for(int j = 0; j < size; j++) {
      int val;
      fscanf(file, "%d", &val);
      *(*(example_arr + j) + i) = val;
    }
  }
  print_matrix_i(example_arr, size);

  double** smoother_conv = get_smoother_conv();
  print_matrix_d(smoother_conv, CONVOLUTION_SIZE);

  free_matrix_d(smoother_conv, CONVOLUTION_SIZE);
  free_matrix_i(example_arr, size);

  fclose(file);
  return 0;
}
