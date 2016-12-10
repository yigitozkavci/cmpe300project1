#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "mpi.h"
#include <stdbool.h>
#include "string.h"
#include "tag_types.h"
#include "debug.h"
#include "slice.h"

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

void debug_row(int* row, int* rank) {
  if(*rank == 1)
    debug_4("%d %d %d", row, row + 1, row + 2, rank);
}

/**
 * In order to smoothen a point, we need 3 rows.
 * row_1: starting address of row 1
 * row_2: starting address of row 2
 * row_3: starting address of row 3
 */
int smoothen_point(int* row_1, int* row_2, int* row_3, int* rank) {
  double smoother_val = (1.0)/9;
  int total = 0;
  for(int i = 0; i < 3; i++) {
    total += *(row_1 + i) + *(row_2 + i) + *(row_3 + i);
  }
  return (int) (total * smoother_val);
}

/*
 * Demands data of three points from either top or bottom slice.
 */
int demand_point_data(int* curr_x, int rank, int* received_vals, char type) {
  MPI_Status status;
  int send_tag;
  int remote_rank;
  if(type == 'u') {
    send_tag = DEMAND_DATA_FROM_UPPER_SLICE_TAG;
    remote_rank = rank - 1;
  } else if(type == 'l') {
    send_tag = DEMAND_DATA_FROM_LOWER_SLICE_TAG;
    remote_rank = rank + 1;
  } else {
    debug_1("[!] WRONG TYPE FOR demanding_point_data", &rank);
    return 0;
  }
  debug_3("I want point data for index %d from %d", curr_x, &remote_rank, &rank);

  int message_exists;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_exists, &status);
  if(message_exists) {
    return 0; // Just as we want to demand point, a message arrived. Returning.
  }

  MPI_Send(
    curr_x,         // initial address of send buffer (choice)
    1,              // number of elements in send buffer (integer)
    MPI_INT,        // type of elements in send buffer (handle)
    remote_rank,    // rank of destination (integer)
    send_tag,       // send tag (integer)
    MPI_COMM_WORLD  // communicator
  );
  MPI_Recv(
    received_vals,  // address of receive buffer
    3,              // number of elements in receive buffer (integer)
    MPI_INT,        // type of elements in receive buffer (handle)
    remote_rank,    // rank of source (integer)
    MPI_ANY_TAG,    // receive tag (integer)
    MPI_COMM_WORLD, // communicator
    &status         // status
  );

  return 1;
}

void master() {
  int proc_size, rank;
  MPI_Status status;
  MPI_Comm_size(MPI_COMM_WORLD, &proc_size);

  int** image = image_from_input();
  int image_slice_size = image_size * image_size / (proc_size - 1);
  int image_slice_type;

  // Giving slaves their slices of image
  for(rank = 1; rank < proc_size; rank++) {
    int* image_slice = get_slice(image, image_size, image_slice_size/image_size, rank - 1);
    /* print_arr(image_slice, image_slice_size); */
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

  // Listening for debug messages and printing them
  for(;;) {
    char* message = malloc(50 * sizeof(char));
    int sender, arg1, arg2, arg3, message_length;
    MPI_Recv(message, 50, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    sender = status.MPI_SOURCE;
    MPI_Get_count(&status, MPI_CHAR, &message_length);
    *(message + message_length) = '\0';
    printf("[%d] ", sender);
    if(status.MPI_TAG == DEBUG_MESSAGE_1_TAG) {
      printf("%s", message);
    } else if(status.MPI_TAG == DEBUG_MESSAGE_2_TAG) {
      MPI_Recv(&arg1, 1, MPI_INT, sender, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD, &status);
      printf(message, arg1);
    } else if(status.MPI_TAG == DEBUG_MESSAGE_3_TAG) {
      MPI_Recv(&arg1, 1, MPI_INT, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Recv(&arg2, 1, MPI_INT, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      printf(message, arg1, arg2);
    }  else if(status.MPI_TAG == DEBUG_MESSAGE_4_TAG) {
      MPI_Recv(&arg1, 1, MPI_INT, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Recv(&arg2, 1, MPI_INT, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Recv(&arg3, 1, MPI_INT, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      printf(message, arg1, arg2, arg3);
    } else if(status.MPI_TAG == JOB_DONE_TAG) {
      debug_2("I've heard that slave %d finished its job.", &sender, &rank);
    }

    printf("\n");
    free(message);
  }
}

/**
 * Slave process
 */
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

  slice_matrix = deserialize_slice(slice, row_count, col_count);
  /* free_matrix_i(slice_matrix, col_count); */

  // Starting smoothing process. After smoothing each point, slave is checking whether
  // there is a message from other slaves
  int start_x, start_y;                   // Starting point of slice processing
  int end_x, end_y;                       // Ending point of slice processing (exclusive)

  // Allocating space for our smoothened_slice
  int** smoothened_slice = (int**)malloc(col_count * sizeof(int*));
  for(int col = 0; col < col_count; col++) {
    *(smoothened_slice + col) = (int*)malloc(row_count * sizeof(int));
  }

  // Setting starting and ending points based on slice types
  start_x = 1;           // Horizontal starting and ending points are the
  end_x = col_count - 1; // same for every slice type
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
    debug_2("Wrong slice type: %d", &slice_type, &rank);
    exit(0);
  }

  // Starting smoothing job
  int curr_x = start_x, curr_y = start_y; // Current point of slice processing
  bool job_finished = false;
  int message_exists;
  for(;;) {
    /* If any of other slave wants demands a point, they send messages.
     * Here, we check whether another slave demands point data from us.
     */
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_exists, &status);
    if(!message_exists) { // Do your own job
      if(job_finished) continue;
      debug_1("Message: ---", &rank);

      /*
       * Checking boundary conditions where we need to rearrange position
       */
      if(curr_x == end_x) {
        if(curr_y == end_y - 1) {
          job_finished = true;
          /* Informing master that my job is done */
          int temp = 1;
          debug_1("My job is done here", &rank);
          MPI_Send(&temp, 1, MPI_INT, 0, JOB_DONE_TAG, MPI_COMM_WORLD);
        } else {
          curr_x = 0;
          curr_y++;
        }
      } else { // We can process (curr_x, curr_y) without hesitation now
        debug_3("Processing point (%d, %d)", &curr_x, &curr_y, &rank);

        /**
         * SLICE POSITIONING
         *
         * * -------------------------- *
         * * HIGH   slice HIGH   row    *
         * * HIGH   slice MIDDLE row    *
         * * HIGH   slice LOW    row    *
         * * -------------------------- *
         * * MIDDLE slice HIGH   row    *
         * * MIDDLE slice MIDDLE row    *
         * * MIDDLE slice LOW    row    *
         * * -------------------------- *
         * * BOTTOM slice HIGH   row    *
         * * BOTTOM slice MIDDLE row    *
         * * BOTTOM slice LOW    row    *
         * * -------------------------- *
         */

        /**
         * DEMANDING CONDITIONS
         *
         * TOP slice
         *   LOW row:
         *     Demand data from lower slice
         *   Else
         *     Standard procedure
         * MIDDLE slice:
         *   LOW row:
         *     Demand data from lower slice
         *   HIGH row:
         *     Demand data from upper slice
         *   Else:
         *     Standard procedure
         * BOTTOM slice
         *   HIGH row:
         *     Demand data from lower slice
         *   Else
         *     Standard procedure
         */

        bool is_low_row = curr_y == end_y - 1;
        bool is_high_row = curr_y == start_y;
        int demand_status; // 0 if demand failed, 1 otherwise.

        // TODO: Test with bigger input
        if(slice_type == SLICE_TYPE_TOP) {
          if(is_low_row) {
            int* row_1 = *(slice_matrix + curr_y - 1);

            int* row_2 = *(slice_matrix + curr_y);

            int* row_3 = malloc(3 * sizeof(int));
            demand_status = demand_point_data(&curr_x, rank, row_3, 'l');
            if(!demand_status) {
              continue;
            }
            
            /* debug_4("%d %d %d", row_1 + curr_x - 1, row_1 + curr_x, row_1 + curr_x + 1, &rank); */
            /* debug_4("%d %d %d", row_2 + curr_x - 1, row_2 + curr_x, row_2 + curr_x + 1, &rank); */
            /* debug_4("%d %d %d", row_3, row_3 + 1, row_3 + 2, &rank); */

            /* Smoothening the point (curr_x, curr_y) */
            int total = smoothen_point(row_1 + curr_x - 1, row_2 + curr_x - 1, row_3, &rank);
          } else {
            /* Standard procedure */
            int* row_1 = *(slice_matrix + curr_y - 1);
            int* row_2 = *(slice_matrix + curr_y);
            int* row_3 = *(slice_matrix + curr_y + 1);

            int total = smoothen_point(row_1 + curr_x - 1, row_2 + curr_x - 1, row_3 + curr_x - 1, &rank);
            debug_2("Total: %d", &total, &rank);
          }
        } else if(slice_type == SLICE_TYPE_MIDDLE) {
          if(is_high_row) {
            int* row_1 = malloc(3 * sizeof(int));
            demand_point_data(&curr_x, rank, row_1, 'u');

            int* row_2 = *(slice_matrix + curr_y);
            int* row_3 = *(slice_matrix + curr_y + 1);

            int total = smoothen_point(row_1, row_2 + curr_x - 1, row_3 + curr_x - 1, &rank);
            /* debug_2("Total: %d", &total, &rank); */
          } else if(is_low_row) {

          } else {

          }
        } else if(slice_type == SLICE_TYPE_BOTTOM) {
          if(is_high_row) {

          } else {

          }
        }
        curr_x++;
      }
    } else { // Send point information according to the message
      debug_1("Message: +++", &rank);
      /*
       * Receiving message. We already know that we have a message
       * from probe above.
       */
      int x_index, y_index;
      int demander_source;
      MPI_Recv(&x_index, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      demander_source = status.MPI_SOURCE;

      /* 
       * Determining y-index based on demand type of data.
       * Slaves can demand data from either higher or lower rows.
       */
      if(status.MPI_TAG == DEMAND_DATA_FROM_UPPER_SLICE_TAG) {
        y_index = end_y - 1;
      } else if(status.MPI_TAG == DEMAND_DATA_FROM_LOWER_SLICE_TAG) {
        y_index = 0;
      } else {
        printf("[!] Wrong tag: %d\n", status.MPI_TAG);
        exit(0);
      }

      debug_4("Process %d wants 3 points around (%d, %d)", &demander_source, &x_index, &y_index, &rank);
      /* Writing point data to send */
      int *points = malloc(sizeof(int) * 3); 
      for(int i = x_index - 1; i <= x_index + 1; i++) {
        *(points + i - x_index + 1) = *(*(slice_matrix + y_index) + i);
      }

      /* Sending point data */
      MPI_Send(points, 3, MPI_INT, demander_source, POINT_DATA_TAG, MPI_COMM_WORLD);
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
