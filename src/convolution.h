#ifndef CONVOLUTION_H_
#define CONVOLUTION_H_

#define CONVOLUTION_SIZE 3

double** get_smoother_conv();
void convolute_i(int** arr, int arr_size, int** convoluter);
void convolute_point_i(int x, int y, int** arr, int** convoluter);
int** convolute_d(int** arr, int arr_size, double** convoluter);
void convolute_point_d(int x, int y, int** arr, double** convoluter, int** convoluted_matrix);

#endif // CONVOLUTION_H_
