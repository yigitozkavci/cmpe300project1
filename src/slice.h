/* Student Name: Yiğit Özkavcı
 * Student Number: 2013400111
 * Compile Status: Compiling
 * Program Status: Working
 *
 * Notes:
 * You can compile the code on any unix/linux platform with typing:
 * $ make
 *
 * Then run with:
 * $ mpiexec -n <n_of_processors> <program> <in_file> <out_file> <threshold>
 */

#ifndef SLICE_H_
#define SLICE_H_

/**********************************************************************
 * Given an array, forms a row_count to col_count matrix based on it,
 **********************************************************************/
int** deserialize_slice(int* slice, int row_count, int col_count);

/**********************************************************************
 * Splits image into given sizes and returns serialized version
 * of the slice in given index.
 **********************************************************************/
int* extract_slice(int** image, int image_size, int image_slice_size, int index);

/**********************************************************************
 * Given a slice matrix, serializes it into an array.
 **********************************************************************/
int* serialize_slice(int** slice, int row_count, int col_count);

#endif // SLICE_H_
