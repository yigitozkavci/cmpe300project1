#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "convolution.h"
#include "mpi.h"

#define DIETAG 1
#define SLICE_TAG 2
#define SLICE_SIZE_TAG 3
#define SLICE_TYPE_TAG 4
#define SLICE_TYPE_TOP 1
#define SLICE_TYPE_MIDDLE 2
#define SLICE_TYPE_BOTTOM 3

const int image_size = 6;
const int smooth_image_size = image_size - 2;
const int binary_image_size = smooth_image_size - 2;

void print_arr(int* arr, int size) {
  for(int i = 0; i < size; i++) {
    if(i != 0) printf(" ");
    printf("%d", *(arr + i));
  }
  printf("\n");
}

int** image_from_input() {
  int** image = (int**)malloc(sizeof(int*) * image_size);
  for(int i = 0; i < image_size; i++) {
    *(image + i) = (int*)malloc(sizeof(int) * image_size);
  }
  FILE* file = fopen("input2.txt", "r");
  for(int i = 0; i < image_size; i++) {
    for(int j = 0; j < image_size; j++) {
      int val;
      fscanf(file, "%d", &val);
      *(*(image + j) + i) = val;
    }
  }
  fclose(file);
  return image;
}

/**
 * Splits image into given sizes and returns the slice in given index.
 */
int* get_slice(int** image, int image_size, int image_slice_size, int index) {
  // We are keeping slice as contiguous mamory because it'll be passed via MPI
  int* slice = (int*)malloc(sizeof(int) * image_slice_size * image_size);
  for(int row = index * image_slice_size; row < (index + 1) * image_slice_size; row++) {
    for(int col = 0; col < image_size; col++) {
      /* printf("addr: %d, val: %d\n", col + row * image_size - index*image_size*image_slice_size, *(*(image + col) + row)); */
      *(slice + col + row * image_size - index*image_size*image_slice_size) = *(*(image + col) + row);
    }
  }
  /* printf("Image Size: %d\nImage Slice Size: %d\nIndex: %d\n", image_size, image_slice_size, index); */
  /* printf("Image: \n"); */
  /* print_matrix_i(image, image_size, "print"); */
  /* printf("Slice: \n"); */
  /* print_arr(slice, image_slice_size * image_size); */
  /* printf("\n\n"); */
  return slice;
}

void master() {
  int proc_size, rank;
  MPI_Status status;
  MPI_Comm_size(MPI_COMM_WORLD, &proc_size);

  int** image = image_from_input();
  int image_slice_size = image_size * image_size / (proc_size - 1);
  int image_slice_type;

  for(rank = 1; rank < proc_size; rank++) {
    int* image_slice = get_slice(image, image_size, image_slice_size/image_size, rank - 1);
    print_arr(image_slice, image_slice_size);
    MPI_Send(&image_slice_size, 1, MPI_INT, rank, SLICE_SIZE_TAG, MPI_COMM_WORLD);
    MPI_Send(image_slice, image_slice_size, MPI_INT, rank, SLICE_TAG, MPI_COMM_WORLD);
    if(rank == 1) {
      image_slice_type = SLICE_TYPE_TOP;
    } else if(rank == proc_size - 1) {
      image_slice_type = SLICE_TYPE_BOTTOM;
    } else {
      image_slice_type = SLICE_TYPE_MIDDLE;
    }
    MPI_Send(&image_slice_type, 1, MPI_INT, rank, SLICE_TYPE_TAG, MPI_COMM_WORLD);
  }

  /* int** smooth_image = convolute_smoothen(image, image_size); */
  /* int** binary_image = convolute_threshold(smooth_image, smooth_image_size); */
  /* print_matrix_i(binary_image, binary_image_size, "binary.txt"); */

  free_matrix_i(image, image_size);
  /* free_matrix_i(smooth_image, smooth_image_size); */
}

void slave() {
  MPI_Status status;
  int work, rank, slice_size;
  int* slice;
  int slice_type;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  for(;;) {
    MPI_Recv(&slice_size, 1, MPI_INT, 0, SLICE_SIZE_TAG, MPI_COMM_WORLD, &status);
    slice = (int*) malloc(slice_size * sizeof(int));
    MPI_Recv(slice, slice_size, MPI_INT, 0, SLICE_TAG, MPI_COMM_WORLD, &status);
    printf("[%d] My Slice: \n", rank);
    print_arr(slice, slice_size);
    MPI_Recv(&slice_type, 1, MPI_INT, 0, SLICE_TYPE_TAG, MPI_COMM_WORLD, &status);
    printf("[%d] My Type: %d\n", rank, slice_type);
    /* if(status.MPI_TAG == DIETAG) { */
    /*   return; */
    /* } else if(status.MPI_TAG == SLICE_SIZE_TAG) { */
    /*   slice_size = work; */
    /*   printf("[%d] Slice size is: %d\n", rank, slice_size); */
    /* } else if(status.MPI_TAG == SLICE_TAG) { */
    /*   slice = &work; */
    /*   print_arr(slice, slice_size); */
    /* } */
  }
}

int main(int argc, char* argv[]) {
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if(rank == 0) {
    master();
  } else {
    slave();
  }

  MPI_Finalize();
  return 0;
}
