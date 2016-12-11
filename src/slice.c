#include "slice.h"
#include <stdlib.h>
#include <stdio.h>

int* extract_slice(int** image, int image_size, int image_slice_size, int index) {
  // We are keeping slice as contiguous mamory because it'll be passed via MPI
  int* slice = malloc(sizeof(int) * image_slice_size * image_size);
  for(int row = index * image_slice_size; row < (index + 1) * image_slice_size; row++) {
    for(int col = 0; col < image_size; col++) {
      *(slice + col + row * image_size - index*image_size*image_slice_size) = *(*(image + col) + row);
    }
  }
  return slice;
}

int** deserialize_slice(int* slice, int row_count, int col_count) {
  // Allocating space for new slice
  int** new_slice = (int**)malloc(row_count * sizeof(int*));
  for(int row = 0; row < row_count; row++) {
    *(new_slice + row) = (int*)malloc(col_count * sizeof(int));
  }

  // Filling new slice matrix
  for(int row = 0; row < row_count; row++) {
    for(int col = 0; col < col_count; col++) {
      *(*(new_slice + row) + col) = *(slice + row * col_count + col);
    }
  }

  free(slice);
  return new_slice;
}

int* serialize_slice(int** slice, int row_count, int col_count) {
  int* flat_slice = (int*)malloc(sizeof(int) * row_count * col_count);
  for(int row = 0; row < row_count; row++) {
    for(int col = 0; col < col_count; col++) {
      int num = *(*(slice + col) + row);
      *(flat_slice + col_count * row + col) = num;    
    }
  }

  return flat_slice;
}
