#include "convolution.h"
#include <stdlib.h>

double** get_smoother_conv() {
  double** smoother_conv = (double**)malloc(sizeof(double*) * CONVOLUTION_SIZE);
  for(int i = 0; i < CONVOLUTION_SIZE; i++) {
    *(smoother_conv + i) = (double*)malloc(sizeof(double) * CONVOLUTION_SIZE);
  }
  for(int i = 0; i < CONVOLUTION_SIZE; i++) {
    for(int j = 0; j < CONVOLUTION_SIZE; j++) {
      *(*(smoother_conv + i) + j) = 1.0/9;
    }
  }
  return smoother_conv;
}
