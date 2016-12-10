#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "mpi.h"
#include <stdbool.h>
#include "string.h"
#include "tag_types.h"
#include "debug.h"
#include "slice.h"

/* There are 3 types of slices: */
#define SLICE_TYPE_TOP 1    /* Meaning slice is at the very top. */
#define SLICE_TYPE_MIDDLE 2 /* Meaning slice is neither at the top or bottom. */
#define SLICE_TYPE_BOTTOM 3 /* Meaning slice is at the very bottom. */

const int image_size = 6;
const int smooth_image_size = image_size - 2;
const int binary_image_size = smooth_image_size - 2;

/**********************************************************************
 * Given a matrix, prints it.
 **********************************************************************/
void print_matrix(int* arr, int size) {
  for(int i = 0; i < size; i++) {
    if(i != 0) printf(" ");
    printf("%d", *(arr + i));
  }
  printf("\n");
}

/**********************************************************************
 * Reads the input and makes a matrix out of it.
 **********************************************************************/
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
  /* if(*rank == 1) */
    /* debug_4("%d %d %d", row, row + 1, row + 2, rank); */
}

/**********************************************************************
 * In order to smoothen a point, we need 3 rows.
 *
 * row_1: starting address of row 1
 * row_2: starting address of row 2
 * row_3: starting address of row 3
 **********************************************************************/
int smoothen_point(int* row_1, int* row_2, int* row_3, int* rank) {
  double smoother_val = (1.0)/9;
  int total = 0;
  for(int i = 0; i < 3; i++) {
    total += *(row_1 + i) + *(row_2 + i) + *(row_3 + i);
  }
  return (int) (total * smoother_val);
}

/**********************************************************************
 * Demands data of three points from either top or bottom slice.
 **********************************************************************/
bool demand_point_data(int* curr_x, int rank, int* received_vals, char type, bool* is_demanded) {
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
    return false;
  }
  /* debug_3("I want point data for index %d from %d", curr_x, &remote_rank, &rank); */

  int message_exists = 0;
  if(*is_demanded == false) {
    MPI_Send(
      curr_x,         // initial address of send buffer (choice)
      1,              // number of elements in send buffer (integer)
      MPI_INT,        // type of elements in send buffer (handle)
      remote_rank,    // rank of destination (integer)
      send_tag,       // send tag (integer)
      MPI_COMM_WORLD  // communicator
    );
    *is_demanded = true; 
  }

  while(1) { 
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_exists, &status);
    if(!message_exists || status.MPI_SOURCE == rank) continue;
    if(status.MPI_SOURCE == remote_rank && status.MPI_TAG == (50 + rank)) {
      break; 
    } else {
      return false;
    }
  }

  /* Only receive messages that are explicit for me. */
  MPI_Recv(
    received_vals,  /* address of receive buffer */
    3,              /* number of elements in receive buffer (integer) */
    MPI_INT,        /* type of elements in receive buffer (handle) */
    remote_rank,    /* rank of source (integer) */
    (50 + rank),    /* receive tag (integer) */
    MPI_COMM_WORLD, /* communicator */
    &status         /* status */
  );

  return true;
}

/**********************************************************************
 * Master process
 ***********************************************************************/
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
    char* message = malloc(100 * sizeof(char));
    int sender, arg1, arg2, arg3, message_length;
    MPI_Recv(message, 100, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
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

/**********************************************************************
* Manages the process of point smoothing. Takes the whole slice
* matrix, the current position and a special case indicator. Special
* case is where we need a row information from another slave.
* For this, we are sending a message using `demand_point_data` method.
*
* SLICE POSITIONING
*
* ------------------------ *
* HIGH   slice HIGH   row  *
* HIGH   slice MIDDLE row  *
* HIGH   slice LOW    row  *
* ------------------------ *
* MIDDLE slice HIGH   row  *
* MIDDLE slice MIDDLE row  *
* MIDDLE slice LOW    row  *
* ------------------------ *
* BOTTOM slice HIGH   row  *
* BOTTOM slice MIDDLE row  *
* BOTTOM slice LOW    row  *
* ------------------------ *

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

***********************************************************************/
void process_rows_for_smoothing(
  int** slice_matrix,    /* Matrix that this slave is responsible of. */

  int curr_x,            /* Current X position that this slave
                            is processing. */

  int curr_y,            /* Current Y position that this slave is
                            processing. */ 

  int special_row,       /* Row that we want from other slaves. This can
                            be 1, 3 or 0 (if there is no need). */

  bool* is_demanded,     /* This boolean keeps track of whether a slave
                            already sent its message demanding point
                            data. This is just to prevent deadlock. */

  bool* should_continue, /* Continue here is the actual `continue`
                            statement. Indicates whether caller method
                            should use continue afterwards. */

  int* total,            /* Result of smoothing process. */

  int* rank               /* Rank of the slave calling this method */
) {

  /* Validation */
  if(special_row != 0 && special_row != 1 && special_row != 3) {
    debug_1("WRONG SPECIAL WRONG IS PASSED!", rank);
    exit(0);
  }

  int *row_1, *row_2, *row_3; /* These rows will be used for smoothing. */

  bool demand_result; /* Here, this variable is very important. `demand_point_data`
                         returns false if some other slave requested data from us
                         while we want to demand data. So we return from that
                         stage and fulfill that slave's demand. */

  bool used_demand = (special_row != 0);  /* If special row is 1 or 3, we should
                                               demand some data from other slaves. */


  if(special_row == 1) {
    row_1 = malloc(3 * sizeof(int));
    demand_result = demand_point_data(&curr_x, *rank, row_1, 'u', is_demanded);
    row_3 = *(slice_matrix + curr_y + 1) + curr_x - 1;
  } else if(special_row == 3) {
    row_3 = malloc(3 * sizeof(int));
    demand_result = demand_point_data(&curr_x, *rank, row_3, 'l', is_demanded);
    row_1 = *(slice_matrix + curr_y - 1) + curr_x - 1;
  }

  row_2 = *(slice_matrix + curr_y) + curr_x - 1;

  if(used_demand && demand_result == false) {
    *should_continue = true;
  } else {
    *is_demanded = false;
  }

  *total = smoothen_point(row_1, row_2, row_3, rank);
  if(special_row == 1) {
    free(row_1);
  } else if(special_row == 3) {
    free(row_3);
  }
}

/**********************************************************************
 * Slave process
 **********************************************************************/
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
  bool is_demanded = false;
  bool someone_need_me;
  for(;;) {

    /* If any of other slave wants demands a point, they send messages.
     * Here, we check whether another slave demands point data from us. */
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_exists, &status);
    someone_need_me = status.MPI_TAG == DEMAND_DATA_FROM_UPPER_SLICE_TAG
                   || status.MPI_TAG == DEMAND_DATA_FROM_LOWER_SLICE_TAG;
    
    if(message_exists && someone_need_me) {

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

      /* Writing point data to send */
      int *points = malloc(sizeof(int) * 3); 
      for(int i = x_index - 1; i <= x_index + 1; i++) {
        *(points + i - x_index + 1) = *(*(slice_matrix + y_index) + i);
      }

      /* debug_2("4 am sending this row to process %d", &demander_source, &rank); */
      debug_row(points, &rank);

      /* Sending point data */
      MPI_Send(points, 3, MPI_INT, demander_source, (50 + demander_source), MPI_COMM_WORLD);

    } else { // Send point information according to the message

      if(job_finished) continue;
      debug_3("Processing (%d, %d)", &curr_x, &curr_y, &rank);
      /*
       * Checking boundary conditions where we need to rearrange position
       */
      if(curr_x == end_x) {
        if(curr_y == end_y - 1) {
          job_finished = true;
          /* Informing master that my job is done */
          int temp = 1333;
          debug_1("My job is done here", &rank);
          MPI_Send(&temp, 1, MPI_INT, 0, JOB_DONE_TAG, MPI_COMM_WORLD);
        } else {
          curr_x = 0;
          curr_y++;
        }

      } else {  // Do your own job
        bool is_low_row = curr_y == end_y - 1;
        bool is_high_row = curr_y == start_y;
        bool should_continue;
        int total;
        int special_row;

        if(slice_type == SLICE_TYPE_TOP) {
          if(is_low_row) {
            special_row = 3;
          } else {
            special_row = 0;
          }
        } else if(slice_type == SLICE_TYPE_MIDDLE) {
          if(is_high_row) {
            special_row = 1;
          } else if(is_low_row) {
            special_row = 3;
          } else {
            special_row = 0;
          }
        } else if(slice_type == SLICE_TYPE_BOTTOM) {
          if(is_high_row) {
            special_row = 1;
          } else {
            special_row = 0;
          }
        }

        process_rows_for_smoothing(slice_matrix, curr_x, curr_y, special_row,
                                   &is_demanded, &should_continue, &total, &rank);
        if(should_continue) {
          debug_1("Continuing...", &rank);
          continue;
        }
        curr_x++;
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
