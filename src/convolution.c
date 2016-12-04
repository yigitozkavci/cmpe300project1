#include "convolution.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "matrix.h"
#include <stdbool.h>

int horizontal_conv[] = {-1, -1, -1, 2, 2, 2, -1, -1, -1};
int vertical_conv[] = {-1, 2, -1, -1, 2, -1, -1, 2, -1};
int obl_positive_conv[] = {-1, -1, 2, -1, 2, -1, 2, -1, -1};
int obl_negative_conv[] = {2, -1, -1, -1, 2, -1, -1, -1, 2};

int** get_convoluter(int type) {
  // Allocating space for our 2d convoluter
  int** conv = (int**)malloc(sizeof(int*) * 3);
  for(int i = 0; i < 3; i++) {
    *(conv + i) = (int*)malloc(sizeof(int) * 3);
  }

  // Choosing convoluter according to type
  int* convoluter;
  switch(type) {
    case 1:
      convoluter = &horizontal_conv[0];
      break;
    case 2:
      convoluter = &vertical_conv[0];
      break;
    case 3:
      convoluter = &obl_positive_conv[0];
      break;
    case 4:
      convoluter = &obl_negative_conv[0];
      break;
  }

  // Filling our convoluter matrix
  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 3; j++) {
      *(*(conv + j) + i) = *(convoluter + i*3 + j);
    }
  }
  return conv;
}

double** get_smoother_conv() {
  // Allocating space
  double** smoother_conv = (double**)malloc(sizeof(double*) * CONVOLUTION_SIZE);
  for(int i = 0; i < CONVOLUTION_SIZE; i++) {
    *(smoother_conv + i) = (double*)malloc(sizeof(double) * CONVOLUTION_SIZE);
  }

  // Filling 2d matrix with values of 1/9
  for(int i = 0; i < CONVOLUTION_SIZE; i++) {
    for(int j = 0; j < CONVOLUTION_SIZE; j++) {
      *(*(smoother_conv + i) + j) = 1.0/9;
    }
  }

  return smoother_conv;
}

int** convolute_threshold(int** arr, int arr_size) {
  // Allocating space
  int** convoluted_matrix = (int**)malloc(sizeof(int*) * (arr_size - 1));
  for(int i = 0; i < arr_size - 1; i++) {
    *(convoluted_matrix + i) = (int*)malloc(sizeof(int) * (arr_size - 1));
  }

  int*** generic_convoluter = (int***)malloc(sizeof(int**) * 4);
  for(int i = 1; i <= 4; i++) {
    *generic_convoluter = get_convoluter(i);
    generic_convoluter++;
  }
  generic_convoluter -= 4;

  int convolution_value; // Instant convolution value which will be compared to threshold
  bool passed_threshold; // Determines whether a convolution value passed threshold
  int binary_value;      // Binary value to put in convoluted matrix

  for(int i = 1; i < arr_size - 1; i++) {
    for(int j = 1; j < arr_size - 1; j++) {
      passed_threshold = false;
      for(int k = 1; k <= 4; k++) {
        convolution_value = convolute_point_i(i, j, arr, *(generic_convoluter + k - 1));
        /* printf("Convolution value: %d\n", convolution_value); */
        if(convolution_value > THRESHOLD) {
          passed_threshold = true;
        }
      }
      if(passed_threshold) {
        binary_value = 255;
      } else {
        binary_value = 0;
      }
      *(*(convoluted_matrix + j - 1) + i - 1) = binary_value;
    }
  }
  return convoluted_matrix;
}

int convolute_point_i(int x, int y, int** arr, int** convoluter) {
  int total = 0;        // Total value of convoluting process
  int convoluter_x = 0; // X position of convoluter matrix
  int convoluter_y = 0; // Y position of convoluter matrix

  // Looping around (x, y) of arr and adding multiplication results to total
  for(int i = x-1; i <= x+1; i++, convoluter_x++) {
    for(int j = y-1; j <= y+1; j++, convoluter_y++) {
      int arr_val = *(*(arr + j) + i);
      int convoluter_val = *(*(convoluter + convoluter_y) + convoluter_x);
      total += arr_val * convoluter_val;
    }
    convoluter_y = 0;
  }

  return total;
}

int** convolute_smoothen(int** arr, int arr_size) {
  // Allocating space
  int** convoluted_matrix = (int**)malloc(sizeof(int*) * (arr_size - 1));
  for(int i = 0; i < arr_size - 1; i++) {
    *(convoluted_matrix + i) = (int*)malloc(sizeof(int) * (arr_size - 1));
  }

  double** smoother_conv = get_smoother_conv();
  // Convoluting around each point
  for(int i = 1; i < arr_size - 1; i++) {
    for(int j = 1; j < arr_size - 1; j++) {
      smoothen_point(i, j, arr, smoother_conv, convoluted_matrix);
    }
  }
  free_matrix_d(smoother_conv, CONVOLUTION_SIZE);

  return convoluted_matrix;
}
