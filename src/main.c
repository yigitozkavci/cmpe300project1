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

#define IMAGE_SIZE 200

/**********************************************************************
 * Reads the input and makes a matrix out of it.
 **********************************************************************/
int** image_from_input() {
  int** image = (int**)malloc(sizeof(int*) * IMAGE_SIZE);
  for(int i = 0; i < IMAGE_SIZE; i++) {
    *(image + i) = (int*)malloc(sizeof(int) * IMAGE_SIZE);
  }
  FILE* file = fopen("input.txt", "r");
  for(int i = 0; i < IMAGE_SIZE; i++) {
    for(int j = 0; j < IMAGE_SIZE; j++) {
      int val;
      fscanf(file, "%d", &val);
      *(*(image + j) + i) = val;
    }
  }
  fclose(file);
  return image;
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

bool is_debug_tag(int tag) {
  return (tag == DEBUG_MESSAGE_1_TAG) ||
         (tag == DEBUG_MESSAGE_2_TAG) ||
         (tag == DEBUG_MESSAGE_3_TAG) ||
         (tag == DEBUG_MESSAGE_4_TAG);
}

/**********************************************************************
 * Master process
 ***********************************************************************/
void master() {
  int proc_size, rank;
  MPI_Status status;
  MPI_Comm_size(MPI_COMM_WORLD, &proc_size);

  int** image = image_from_input();
  int image_slice_size = IMAGE_SIZE * IMAGE_SIZE / (proc_size - 1);
  int image_slice_type;

  // Giving slaves their slices of image
  for(rank = 1; rank < proc_size; rank++) {
    int* image_slice = extract_slice(image, IMAGE_SIZE, image_slice_size/IMAGE_SIZE, rank - 1);
    MPI_Send(&image_slice_size, 1, MPI_INT, rank, SLICE_SIZE_TAG, MPI_COMM_WORLD);
    *(image_slice + image_slice_size) = IMAGE_SIZE;
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

  for(int i = 0; i < IMAGE_SIZE; i++) {
    free(*(image + i));
  }
  free(image);

  // Listening for debug messages and printing them
  int job_to_finish = 1;
  int message_exists;
  int job_finished_count = 0;

  /* Allocating space for the new smoothened image. */
  int** new_image = (int**)malloc(sizeof(int*) * IMAGE_SIZE);
  for(int i = 0; i < IMAGE_SIZE; i++) {
    *(new_image + i) = (int*)malloc(sizeof(int) * IMAGE_SIZE);
    for(int j = 0; j < IMAGE_SIZE; j++) {
      *(*(new_image + i) + j) = 0;
    }
  }

  for(;;) {
    if(job_to_finish != proc_size) {
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_exists, &status);
      if(!message_exists && job_finished_count == proc_size - 1) {
        printf("Smoothing is completed.\n");
        int temp;
        MPI_Request request;
        MPI_Isend(&temp, 1, MPI_INT, job_to_finish, FINISH_SMOOTHING_TAG, MPI_COMM_WORLD, &request);
        job_to_finish++;
        continue;
      }
    } else {
      printf("Master is finished!\n\n\n");
      FILE *f;
      f = fopen("out.txt", "w");
      for(int row = 0; row < IMAGE_SIZE; row++) {
        for(int col = 0; col < IMAGE_SIZE; col++) {
          fprintf(f, "%d ", *(*(new_image + col) + row));
        }
        fprintf(f, "\n");
      }
      fprintf(f, "\n");
      break;
    }

    int sender, arg1, arg2, arg3, message_length, message_exists;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_exists, &status);

    sender = status.MPI_SOURCE;
    if(message_exists) {
      if(is_debug_tag(status.MPI_TAG)) {
        int msg_length;
        MPI_Get_count(&status, MPI_CHAR, &msg_length);
        char* message = malloc(msg_length * sizeof(char));
        MPI_Recv(message, msg_length, MPI_CHAR, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
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
        } else if(status.MPI_TAG == DEBUG_MESSAGE_4_TAG) {
          MPI_Recv(&arg1, 1, MPI_INT, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
          MPI_Recv(&arg2, 1, MPI_INT, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
          MPI_Recv(&arg3, 1, MPI_INT, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
          printf(message, arg1, arg2, arg3);
        }
        printf("\n");
        free(message);
      } else if(status.MPI_TAG == JOB_DONE_TAG) {
        /* Receiving slice array length from slave */
        int slice_arr_length;
        MPI_Recv(&slice_arr_length, 1, MPI_INT, sender, JOB_DONE_TAG, MPI_COMM_WORLD, &status);

        /* Receiving deserialized slice array from slave */
        int* slice_arr = malloc(sizeof(int) * slice_arr_length);
        MPI_Recv(slice_arr, slice_arr_length, MPI_INT, sender, FOLLOWING_JOB_DONE_TAG, MPI_COMM_WORLD, &status);

        printf("I've heard that slave %d finished its job, here is the result:\n", sender);
        /* Putting that serialized slice array to new_image. */
        int slice_row_count = IMAGE_SIZE / (proc_size - 1);
        int slice_col_count = IMAGE_SIZE;
        for(int row = 0; row < slice_row_count; row++) {
          for(int col = 0; col < slice_col_count; col++) {
            int slice_row_offset = (sender - 1) * slice_row_count;
            int arr_val = *(slice_arr + row * slice_col_count + col);
            *(*(new_image + col) + row + slice_row_offset) = arr_val;
          }
        }
        job_finished_count++;
      }
    }
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
    row_1 = malloc(sizeof(int) * 3);
    demand_result = demand_point_data(&curr_x, *rank, row_1, 'u', is_demanded);
    row_3 = *(slice_matrix + curr_y + 1) + curr_x - 1;
  } else if(special_row == 3) {
    row_3 = malloc(sizeof(int) * 3);
    demand_result = demand_point_data(&curr_x, *rank, row_3, 'l', is_demanded);
    row_1 = *(slice_matrix + curr_y - 1) + curr_x - 1;
  } else {
    row_1 = *(slice_matrix + curr_y - 1) + curr_x - 1;
    row_3 = *(slice_matrix + curr_y + 1) + curr_x - 1;
  }


  row_2 = *(slice_matrix + curr_y) + curr_x - 1;

  if(used_demand && demand_result == false) {
    *should_continue = true;
  } else {
    *should_continue = false;
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
  int rank, slice_size, slice_type, row_count, col_count;
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

  // Starting smoothing process. After smoothing each point, slave is checking whether
  // there is a message from other slaves
  int start_x, start_y;                   // Starting point of slice processing
  int end_x, end_y;                       // Ending point of slice processing (exclusive)

  // Allocating space for our smoothened_slice
  int** smoothened_slice = (int**)malloc(col_count * sizeof(int*));
  for(int col = 0; col < col_count; col++) {
    *(smoothened_slice + col) = (int*)malloc(row_count * sizeof(int));
    for(int row = 0; row < row_count; row++) {
      *(*(smoothened_slice + col) + row) = 0;
    }
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
  bool should_continue;
  for(;;) {

    /* If any of other slave wants demands a point, they send messages.
     * Here, we check whether another slave demands point data from us. */
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_exists, &status);
    someone_need_me = status.MPI_TAG == DEMAND_DATA_FROM_UPPER_SLICE_TAG
                   || status.MPI_TAG == DEMAND_DATA_FROM_LOWER_SLICE_TAG;

    if(status.MPI_TAG == FINISH_SMOOTHING_TAG) {
      for(int row = 0; row < row_count; row++) {
         free(*(slice_matrix + row)); 
      }
      free(slice_matrix);
      printf("Slave is returning.\n");
      return;
    }

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

      /* Sending point data */
      MPI_Send(points, 3, MPI_INT, demander_source, (50 + demander_source), MPI_COMM_WORLD);
      free(points);

    } else { // Do your own job
      if(job_finished) continue;

      /*
       * Checking boundary conditions where we need to rearrange position
       */
      if(curr_x == end_x) {
        if(curr_y == end_y - 1) {
          job_finished = true;
          /* Informing master that my job is done */
          int slice_size = row_count * col_count;
          MPI_Send(&slice_size, 1, MPI_INT, 0, JOB_DONE_TAG, MPI_COMM_WORLD);
          int* slice = serialize_slice(smoothened_slice, row_count, col_count);
          MPI_Send(slice, row_count * col_count, MPI_INT, 0, FOLLOWING_JOB_DONE_TAG, MPI_COMM_WORLD);
        } else {
          curr_x = 1;
          curr_y++;
        }
      } else {  // Do your own job

        bool is_low_row = curr_y == end_y - 1;
        bool is_high_row = curr_y == start_y;
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
        } else {
          printf("Wrong slice type: %d", slice_type);
          exit(0);
        }

        process_rows_for_smoothing(slice_matrix, curr_x, curr_y, special_row,
                                   &is_demanded, &should_continue, &total, &rank);

        if(should_continue) {
          continue;
        }
        *(*(smoothened_slice + curr_x) + curr_y) = total;
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
    printf("Master is finished.\n");
  } else {
    slave();
    printf("Slave is finished.\n");
  }

  printf("Finalizing %d\n", rank);
  MPI_Finalize();
}
