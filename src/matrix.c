#include "matrix.h"
#include <stdlib.h>
#include <stdio.h>

void print_matrix_d(double** matrix, int size) {
  for(int i = 0; i < size; i++) {
    for(int j = 0; j < size; j++) {
      if(j != 0) printf(" ");
      printf("%f", *(*(matrix + j) + i));
    }
    printf("\n");
  }
}

void print_matrix_i(int** matrix, int size) {
  for(int i = 0; i < size; i++) {
    for(int j = 0; j < size; j++) {
      if(j != 0) printf(" ");
      printf("%d", *(*(matrix + j) + i));
    }
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
