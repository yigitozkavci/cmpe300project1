#include "matrix.h"
#include <stdlib.h>
#include <stdio.h>

void print_arr_i(int* arr, int size) {
  for(int i = 0; i < size; i++) {
    if(i != 0) printf(" ");
    printf("%d", arr[i]);
  }
}

void print_arr_d(double* arr, int size) {
  for(int i = 0; i < size; i++) {
    if(i != 0) printf(" ");
    printf("%f", arr[i]);
  }
}

void print_matrix_i(int** matrix, int size) {
  for(int i = 0; i < size; i++) {
    print_arr_i(*(matrix + i), size);
    printf("\n");
  }
}

void print_matrix_d(double** matrix, int size) {
  for(int i = 0; i < size; i++) {
    print_arr_d(*(matrix + i), size);
    printf("\n");
  }
}

void free_matrix_d(double** matrix, int size) {
  for(int i = 0; i < size; i++) {
    free(*(matrix + i));
  }
  free(matrix);
}

void free_matrix_i(int** matrix, int size) {
  for(int i = 0; i < size; i++) {
    free(*(matrix + i));
  }
  free(matrix);
}
