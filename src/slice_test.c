#include "slice.h"
#include <stdlib.h>
#include "matrix.h"


int main() {
  int row_size = 3;
  int col_size = 2;
  int** image = (int**)malloc(sizeof(int*) * col_size);
  for(int col = 0; col < col_size; col++) {
    *(image + col) = (int*)malloc(sizeof(int) * row_size);
  }

  print_matrix_i(image, row_size, col_size, "print");
  return 0;
}
