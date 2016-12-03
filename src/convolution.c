#include "convolution.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int horizontal_conv[] = {-1, -1, -1, 2, 2, 2, -1, -1, -1};
int vertical_conv[] = {-1, 2, -1, -1, 2, -1, -1, 2, -1};
int obl_positive_conv[] = {-1, -1, 2, -1, 2, -1, 2, -1, -1};
int obl_negative_conv[] = {2, -1, -1, -1, 2, -1, -1, -1, 2};

int** get_convoluter(int type) {
  // Allocating space for our 2d convoluter
  int** conv = (int**)malloc(sizeof(int*) * 3);
  for(int i = 0; i < 3 - 1; i++) {
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

int** convolute_i(int** arr, int arr_size, int** convoluter) {
  // Allocating space
  int** convoluted_matrix = (int**)malloc(sizeof(int*) * (arr_size - 1));
  for(int i = 0; i < arr_size - 1; i++) {
    *(convoluted_matrix + i) = (int*)malloc(sizeof(int) * (arr_size - 1));
  }

  // Convoluting around each point
  for(int i = 1; i < arr_size - 1; i++) {
    for(int j = 1; j < arr_size - 1; j++) {
      convolute_point_i(i, j, arr, convoluter, convoluted_matrix);
    }
  }

  return convoluted_matrix;
}

void convolute_point_i(int x, int y, int** arr, int** convoluter, int** convoluted_matrix) {
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

  // Becaue we are creating a smaller matrix for resulting convoluted matrix,
  // we are doing it by shifting array indexes by 1.
  // Ex. For a matrix of size 200x200, after convolution we will have 198x198
  *(*(convoluted_matrix + y - 1) + x - 1) = total;
}

int** convolute_d(int** arr, int arr_size, double** convoluter) {
  // Allocating space
  int** convoluted_matrix = (int**)malloc(sizeof(int*) * (arr_size - 1));
  for(int i = 0; i < arr_size - 1; i++) {
    *(convoluted_matrix + i) = (int*)malloc(sizeof(int) * (arr_size - 1));
  }

  // Convoluting around each point
  for(int i = 1; i < arr_size - 1; i++) {
    for(int j = 1; j < arr_size - 1; j++) {
      convolute_point_d(i, j, arr, convoluter, convoluted_matrix);
    }
  }

  return convoluted_matrix;
}

void convolute_point_d(int x, int y, int** arr, double** convoluter, int** convoluted_matrix) {
  double total = 0;     // Total value of convoluting process
  int convoluter_x = 0; // X position of convoluter matrix 
  int convoluter_y = 0; // Y position of convoluter matrix

  // Looping around (x, y) of arr and adding multiplication results to total
  for(int i = x-1; i <= x+1; i++, convoluter_x++) {
    for(int j = y-1; j <= y+1; j++, convoluter_y++) {
      int arr_val = *(*(arr + j) + i);
      double convoluter_val = *(*(convoluter + convoluter_y) + convoluter_x);
      total += arr_val * convoluter_val;
    }
    convoluter_y = 0;
  }

  // Since this is convolution uses a double convoluter, we are casting result to int
  int result;
  if(fabs(total - round(total)) < 0.001) {
    result = (int) round(total);
  } else {
    result = (int) total;
  }

  // Becaue we are creating a smaller matrix for resulting convoluted matrix,
  // we are doing it by shifting array indexes by 1.
  // Ex. For a matrix of size 200x200, after convolution we will have 198x198
  *(*(convoluted_matrix + y - 1) + x - 1) = result;
}
