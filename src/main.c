#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "convolution.h"
#include "mpi.h"
#include <stdbool.h>

#define DIETAG 1
#define SLICE_TAG 2
#define SLICE_SIZE_TAG 3
#define SLICE_TYPE_TAG 4
#define DEMAND_DATA_FROM_UPPER_SLICE_TAG 5
#define DEMAND_DATA_FROM_LOWER_SLICE_TAG 6
#define POINT_DATA_TAG 7
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

/*
 * Demands data of three points from either top or bottom slice.
 */
void demand_point_data(int* curr_x, int rank, int* received_vals, char type, MPI_Status status) {
  printf("[%d] Demanding data for index %d as process \n", rank, *curr_x);
  int send_tag;
  int remote_rank;
  if(type == 'u') {
    send_tag = DEMAND_DATA_FROM_UPPER_SLICE_TAG;
    remote_rank = rank - 1;
  } else if(type == 'l') {
    send_tag = DEMAND_DATA_FROM_LOWER_SLICE_TAG;
    remote_rank = rank + 1;
  }
  printf("Sending message\n");
  MPI_Sendrecv(
    curr_x,            // initial address of send buffer (choice)
    1,                  // number of elements in send buffer (integer)
    MPI_INT,            // type of elements in send buffer (handle)
    remote_rank,           // rank of destination (integer)
    send_tag, // send tag (integer)
    received_vals,      // address of receive buffer
    3,                  // number of elements in receive buffer (integer)
    MPI_INT,            // type of elements in receive buffer (handle)
    remote_rank,           // rank of source (integer)
    MPI_ANY_TAG,       // receive tag (integer)
    MPI_COMM_WORLD,     // communicator
    &status             // status
  );
  printf("Received message!\n");
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
    *(image_slice + image_slice_size) = image_size;
    MPI_Send(image_slice, image_slice_size + 1, MPI_INT, rank, SLICE_TAG, MPI_COMM_WORLD);
    if(rank == 1) {
      image_slice_type = SLICE_TYPE_TOP;
    } else if(rank == proc_size - 1) {
      image_slice_type = SLICE_TYPE_BOTTOM;
    } else {
      image_slice_type = SLICE_TYPE_MIDDLE;
    }
    MPI_Send(&image_slice_type, 1, MPI_INT, rank, SLICE_TYPE_TAG, MPI_COMM_WORLD);
  }
}

int** deserialize_slice(int* slice, int row_count, int col_count) {
  // Allocating space for new slice
  int** new_slice = (int**)malloc(col_count * sizeof(int*));
  for(int col = 0; col < col_count; col++) {
    *(new_slice + col) = (int*)malloc(row_count * sizeof(int));
  }

  // Filling new slice matrix
  for(int row = 0; row < row_count; row++) {
    for(int col = 0; col < col_count; col++) {
      *(*(new_slice + col) + row) = *(slice + row * col_count + col);
    }
  }

  free(slice);
  return new_slice;
}

void slave() {
  MPI_Status status;
  int work, rank, slice_size, slice_type, row_count, col_count;
  int* slice;         // Slice that this slave is going to work on. It's an array.
  int** slice_matrix; // We are receiving slice as an array but then deserializing it to matrix.

  // Setting rank
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Receiving slice size
  MPI_Recv(&slice_size, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

  // Receiving slice on our allocated slice space.
  //
  // Assume a 6x6 image seperated into 3 slices:
  // 1 slice has 12 points and we are sending 13 integers being first 12 is
  // slice data, and 13th is column count which is 6 since image is 6x6.
  slice = (int*) malloc((slice_size + 1) * sizeof(int)); // Allocating space for slave's slice
  MPI_Recv(slice, slice_size + 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  col_count = *(slice + slice_size);
  row_count = slice_size/col_count; 

  // Receiving type of slice which indicates whether it's at the top, bottom or middle
  MPI_Recv(&slice_type, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  /* printf("[%d] My Type: %d\n", rank, slice_type); */

  slice_matrix = deserialize_slice(slice, row_count, col_count);
  /* free_matrix_i(slice_matrix, col_count); */

  // Starting smoothing process. After smoothing each point, slave is checking whether
  // there is a message from other slaves
  int operation_count;
  int start_x, start_y;                   // Starting point of slice processing
  int end_x, end_y;                       // Ending point of slice processing (exclusive)

  // Setting starting and ending points based on slice types
  start_x = 1;           // Horizontal starting and ending points are the
  end_x = col_count - 1; // same for every slice
  if(slice_type == SLICE_TYPE_MIDDLE) {
    start_y = 0;
    end_y = row_count;
  } else if(slice_type == SLICE_TYPE_TOP) {
    start_y = 1;
    end_y = row_count;
  } else if(slice_type == SLICE_TYPE_BOTTOM) {
    start_y = 0;
    end_y = row_count - 1;
  } else {
    printf("[!] Wrong slice type: %d\n", slice_type);
    exit(0);
  }

  // Starting smoothing job
  int curr_x = start_x, curr_y = start_y; // Current point of slice processing
  bool job_finished = false;
  int message_exists;
  printf("[%d] Slice I am working on: \n", rank);
  printf("%d, %d\n", row_count, col_count);
  print_matrix_i(slice_matrix, row_count, col_count, "print");
  for(;;) {
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_exists, &status);
    if(!message_exists && !job_finished) { // Do your own job
      if(curr_x == end_x) { // Boundary conditions where we need to rearrange position
        if(curr_y == end_y) {
          job_finished = true;
        } else {
          curr_x = 0;
          curr_y++;
        }
      } else { // We can process (curr_x, curr_y) without hesitation now
        // Check if we need to send a message for boundary points
        if(slice_type == SLICE_TYPE_TOP) {
          if(curr_y == end_y - 1) {
            int received_vals;
            demand_point_data(&curr_x, rank, &received_vals, 'l', status);
            printf("[%d] Received these points: \n", rank);
            print_arr(&received_vals, 3);
          }
          printf("wee!\n");
          curr_x++;
        } else if(slice_type == SLICE_TYPE_MIDDLE) {

        } else if(slice_type == SLICE_TYPE_BOTTOM) {

        }
      }
    } else { // Send information according to the message
      int x_index;
      int demander_source;
      MPI_Recv(&x_index, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      demander_source = status.MPI_SOURCE;
      printf("[%d] Source %d wants my x_index:%d data\n", rank, demander_source, x_index);
      if(status.MPI_TAG == DEMAND_DATA_FROM_UPPER_SLICE_TAG) {
        ;
      } else if(status.MPI_TAG == DEMAND_DATA_FROM_LOWER_SLICE_TAG) {
        int *points = malloc(sizeof(int) * 3); 
        for(int i = x_index - 1; i <= x_index + 1; i++) {
          printf("Setting point value: %d\n", *(*(slice_matrix + i) + 0));
          *(points + i - x_index + 1) = *(*(slice_matrix + i) + 0);
        }
        printf("Sent these points: \n");
        printf("%d %d %d\n", *points, *(points + 1), *(points + 2));
        MPI_Send(points, 3, MPI_INT, demander_source, POINT_DATA_TAG, MPI_COMM_WORLD);
      } else {
        printf("[!] Wrong tag: %d\n", status.MPI_TAG);
        exit(0);
      }
    }
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
