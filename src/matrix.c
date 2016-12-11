#include "matrix.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"

void print_matrix_d(double** matrix, int size) {
  for(int i = 0; i < size; i++) {
    for(int j = 0; j < size; j++) {
      if(j != 0) printf(" ");
      printf("%f", *(*(matrix + j) + i));
    }
    printf("\n");
  }
}

void print_matrix_i(int** matrix, int rows, int cols, char* filename, int* rank) {
  if(*rank != 1) return;
  debug_1("\n\n", rank);
  for(int i = 0; i < rows; i++) {
    for(int j = 0; j < cols; j++) {
      char* str;
      debug_2("%d", (*(matrix + j) + i), rank);
    }
    debug_1("\n", rank);
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
