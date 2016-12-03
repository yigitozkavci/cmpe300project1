#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "convolution.h"

const int image_size = 200;

int main() {
  printf("Hello!\n");
  int smooth_image_size = image_size - 2;
  int binary_image_size = smooth_image_size - 2;

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
  int** smooth_image = convolute_smoothen(image, image_size);
  int** binary_image = convolute_threshold(smooth_image, smooth_image_size);
  print_matrix_i(image, image_size, "original.txt");
  print_matrix_i(smooth_image, smooth_image_size, "smooth.txt");
  print_matrix_i(binary_image, binary_image_size, "binary.txt");

  free_matrix_i(image, image_size);
  free_matrix_i(smooth_image, smooth_image_size);

  fclose(file);
  return 0;
}
