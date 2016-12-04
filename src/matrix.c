#include "matrix.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void print_matrix_d(double** matrix, int size) {
  for(int i = 0; i < size; i++) {
    for(int j = 0; j < size; j++) {
      if(j != 0) printf(" ");
      printf("%f", *(*(matrix + j) + i));
    }
    printf("\n");
  }
}

void print_matrix_i(int** matrix, int rows, int cols, char* filename) {
  FILE *f;
  if(strncmp(filename, "print", 5) == 0)
    f = fopen(filename, "w");
  for(int i = 0; i < rows; i++) {
    for(int j = 0; j < cols; j++) {
      if(strncmp(filename, "print", 5) == 0) {
        if(j != 0) printf(" ");
        printf("%d", *(*(matrix + j) + i));
      } else {
        if(j != 0) fprintf(f, " ");
        fprintf(f, "%d", *(*(matrix + j) + i));
      }
    }
    if(strncmp(filename, "print", 5) == 0) {
      printf("\n");
    } else {
      fprintf(f, "\n");
    }
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
