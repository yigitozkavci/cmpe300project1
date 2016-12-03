#ifndef CONVOLUTION_H_
#define CONVOLUTION_H_

#define CONVOLUTION_SIZE 3
#define THRESHOLD 50

/**
 * Allocates a 2D convoluter according to given type.
 * Convoluter definitions are hardcoded in convolution.c file.
 */
int** get_convoluter(int type);

/*
 * Generates a smoother convoluter. It's a 3x3 matrix with all values 1/9.
 */
double** get_smoother_conv();

/*
 * Convolutes given matrix with given integer convoluter.
 */
int** convolute_i(int** arr, int arr_size, int** convoluter);

/*
 * Convolutes the point (x, y) of arr with given convoluter.
 */
int convolute_point_i(int x, int y, int** arr, int** convoluter);

/*
 * Smoothens the given matrix.
 */
int** convolute_smoothen(int** arr, int arr_size);

/*
 * Using 4 generic convoluters, decides a value of either 0 or 255
 * based on a threshold.
 */
int** convolute_threshold(int** arr, int arr_size);

/*
 * Convolutes the point (x, y) of arr with given double convoluter.
 */
void smoothen_point(int x, int y, int** arr, double** convoluter, int** convoluted_matrix);

#endif // CONVOLUTION_H_
