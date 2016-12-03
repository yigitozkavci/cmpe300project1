#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "convolution.h"
#include "mpi.h"

const int image_size = 200;
const int smooth_image_size = image_size - 2;
const int binary_image_size = smooth_image_size - 2;

int** image_from_input() {
  int** image = (int**)malloc(sizeof(int*) * image_size);
  for(int i = 0; i < image_size; i++) {
    *(image + i) = (int*)malloc(sizeof(int) * image_size);
  }
  FILE* file = fopen("input.txt", "r");
  for(int i = 0; i < image_size; i++) {
    for(int j = 0; j < image_size; j++) {
      int val;
      fscanf(file, "%d", &val);
      *(*(image + j) + i) = val;
    }
  }
  fclose(file);
  return image;
}

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);
  printf("Whoa");
  MPI_Finalize();

  int** image = image_from_input();

  int** smooth_image = convolute_smoothen(image, image_size);
  int** binary_image = convolute_threshold(smooth_image, smooth_image_size);
  print_matrix_i(binary_image, binary_image_size, "binary.txt");

  free_matrix_i(image, image_size);
  free_matrix_i(smooth_image, smooth_image_size);

  return 0;
}
