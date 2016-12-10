#ifndef SLICE_H_
#define SLICE_H_

/**
 * Given an array, forms a row_count to col_count matrix based on it,
 */
int** deserialize_slice(int* slice, int row_count, int col_count);

/**
 * Splits image into given sizes and returns serialized version of the slice in given index.
 */
int* get_slice(int** image, int image_size, int image_slice_size, int index);

#endif // SLICE_H_
