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

void print_matrix_i(int** matrix, int size, char* filename) {
  FILE *f = fopen(filename, "w");
  for(int i = 0; i < size; i++) {
    for(int j = 0; j < size; j++) {
      if(j != 0) fprintf(f, " ");
      fprintf(f, "%d", *(*(matrix + j) + i));
    }
    fprintf(f, "\n");
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
