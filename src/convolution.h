#ifndef CONVOLUTION_H_
#define CONVOLUTION_H_

#define CONVOLUTION_SIZE 3

/*
 * Generates a smoother convoluter. It's a 3x3 matrix with all values 1/9.
 */
double** get_smoother_conv();

/*
 * Convolutes given matrix with given integer convoluter.
 */
void convolute_i(int** arr, int arr_size, int** convoluter);

/*
 * Convolutes the point (x, y) of arr with given convoluter.
 */
void convolute_point_i(int x, int y, int** arr, int** convoluter);

/*
 * Convolutes given matrix with given double convoluter.
 */
int** convolute_d(int** arr, int arr_size, double** convoluter);

/*
 * Convolutes the point (x, y) of arr with given double convoluter.
 */
void convolute_point_d(int x, int y, int** arr, double** convoluter, int** convoluted_matrix);

#endif // CONVOLUTION_H_
