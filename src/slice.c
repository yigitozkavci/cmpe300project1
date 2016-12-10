#include "slice.h"
#include <stdlib.h>

/**
 * Splits image into given sizes and returns serialized version of the slice in given index.
 */
int* get_slice(int** image, int image_size, int image_slice_size, int index) {
  // We are keeping slice as contiguous mamory because it'll be passed via MPI
  int* slice = (int*)malloc(sizeof(int) * image_slice_size * image_size);
  for(int row = index * image_slice_size; row < (index + 1) * image_slice_size; row++) {
    for(int col = 0; col < image_size; col++) {
      *(slice + col + row * image_size - index*image_size*image_slice_size) = *(*(image + col) + row);
    }
  }
  return slice;
}


/**
 * Given an array, forms a row_count to col_count matrix based on it,
 */
int** deserialize_slice(int* slice, int row_count, int col_count) {
  // Allocating space for new slice
  int** new_slice = (int**)malloc(col_count * sizeof(int*));
  for(int col = 0; col < col_count; col++) {
    *(new_slice + col) = (int*)malloc(row_count * sizeof(int));
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
