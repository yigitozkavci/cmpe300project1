#include <stdlib.h>
#include <stdio.h>
#include "matrix.h"
#include "mpi.h"
#include <stdbool.h>
#include "string.h"
#include "tag_types.h"
#include "debug.h"
#include "slice.h"
#include "util.h"

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
  int smoothing_job_to_finish = 1;
  int message_exists;
  int smoothing_finished_count = 0;
  int thresholding_finished_count = 0;
  bool thresholding_jobs_sent = false;


  /* Allocating space for the new smoothened and thresholded image. */
  int** master_smoothened_image = util_alloc_matrix(IMAGE_SIZE, IMAGE_SIZE);
  int** master_thresholded_image = util_alloc_matrix(IMAGE_SIZE, IMAGE_SIZE);

  /**********************************************************************
   * What happens here is this:
   *
   * Master has 2 main jobs that it needs to perform infinitely:
   * - If receives a debug message, prints it.
   * - If a slave informs master about job complete, it master takes act  accordingly.
   *   - If slave completes a SMOOTHING stage, master increases
   *     smoothing_finished_count. If this count equals to slave count, this
   *     means that master needs to transmit stage of all slaves from
   *     SMOOTHING to THRESHOLDING.
   *   - If a slave completes a THRESHOLDING stage, master increases
   *     thresholding_finished_count. If this count equals to slave count, this
   *     means that master needs to send JOB_DONE to all slaves, which kills
   *     their processes.
   *     
   **********************************************************************/
  for(;;) {
    if(job_to_finish == proc_size) {
      printf("Master is finished!\n\n\n");
      FILE *f;
      f = fopen("smoothened.txt", "w");
      for(int row = 0; row < IMAGE_SIZE; row++) {
        for(int col = 0; col < IMAGE_SIZE; col++) {
          fprintf(f, "%d ", *(*(master_smoothened_image + col) + row));
        }
        fprintf(f, "\n");
      }
      fprintf(f, "\n");

      f = fopen("thresholded.txt", "w");
      for(int row = 0; row < IMAGE_SIZE; row++) {
        for(int col = 0; col < IMAGE_SIZE; col++) {
          fprintf(f, "%d ", *(*(master_thresholded_image + col) + row));
        }
        fprintf(f, "\n");
      }
      fprintf(f, "\n");
      break;
    } else if(smoothing_job_to_finish == proc_size && !thresholding_jobs_sent) {
      printf("All smoothing jobs are finished. Sending START_THRESHOLDING_TAG to all\n");
      int temp;
      MPI_Request request;
      for(int i = 1; i < proc_size; i++) {

        printf("Sending START_THRESHOLDING_TAG to %d\n", i);
        MPI_Isend(&temp, 1, MPI_INT, i, START_THRESHOLDING_TAG, MPI_COMM_WORLD, &request);
      }
      thresholding_jobs_sent = true;
    } else {
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message_exists, &status);
      if(!message_exists && thresholding_finished_count == proc_size - 1) {
        printf("Thresholding is completed.\n");
        int temp;
        MPI_Request request;
        MPI_Isend(&temp, 1, MPI_INT, job_to_finish, FINISH_THRESHOLDING_TAG, MPI_COMM_WORLD, &request);
        job_to_finish++;
        continue;
      } else if(!message_exists && smoothing_finished_count == proc_size - 1 && !thresholding_jobs_sent) {
        printf("Smoothing is completed for slave %d.\n", smoothing_job_to_finish);
        int temp;
        MPI_Request request;
        MPI_Isend(&temp, 1, MPI_INT, smoothing_job_to_finish, FINISH_SMOOTHING_TAG, MPI_COMM_WORLD, &request);
        smoothing_job_to_finish++;
        continue;
      }
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
      } else if(status.MPI_TAG == SMOOTHING_DONE_TAG) {
        /* Receiving slice array length from slave */
        int slice_arr_length;
        MPI_Recv(&slice_arr_length, 1, MPI_INT, sender, SMOOTHING_DONE_TAG, MPI_COMM_WORLD, &status);

        /* Receiving deserialized slice array from slave */
        int* slice_arr = malloc(sizeof(int) * slice_arr_length);
        MPI_Recv(slice_arr, slice_arr_length, MPI_INT, sender, FOLLOWING_SMOOTHING_DONE_TAG, MPI_COMM_WORLD, &status);

        printf("I've heard that slave %d finished its job, here is the result:\n", sender);

        /* Putting that serialized slice array to master_smoothened_image. */
        int slice_row_count = IMAGE_SIZE / (proc_size - 1);
        int slice_col_count = IMAGE_SIZE;
        for(int row = 0; row < slice_row_count; row++) {
          for(int col = 0; col < slice_col_count; col++) {
            int slice_row_offset = (sender - 1) * slice_row_count;
            int arr_val = *(slice_arr + row * slice_col_count + col);
            *(*(master_smoothened_image + col) + row + slice_row_offset) = arr_val;
          }
        }
        smoothing_finished_count++;
      } else if(status.MPI_TAG == THRESHOLDING_DONE_TAG) {
        /* Receiving slice array length from slave */
        int slice_arr_length;
        MPI_Recv(&slice_arr_length, 1, MPI_INT, sender, THRESHOLDING_DONE_TAG, MPI_COMM_WORLD, &status);

        /* Receiving deserialized slice array from slave */
        int* slice_arr = malloc(sizeof(int) * slice_arr_length);
        MPI_Recv(slice_arr, slice_arr_length, MPI_INT, sender, FOLLOWING_THRESHOLDING_DONE_TAG, MPI_COMM_WORLD, &status);

        printf("I've heard that slave %d finished its thresholding, here is the result:\n", sender);

        /* Putting that serialized slice array to master_thresholded_image. */
        int slice_row_count = IMAGE_SIZE / (proc_size - 1);
        int slice_col_count = IMAGE_SIZE;
        for(int row = 0; row < slice_row_count; row++) {
          for(int col = 0; col < slice_col_count; col++) {
            int slice_row_offset = (sender - 1) * slice_row_count;
            int arr_val = *(slice_arr + row * slice_col_count + col);
            *(*(master_thresholded_image + col) + row + slice_row_offset) = arr_val;
          }
        }
        thresholding_finished_count++;
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

  int** smoothened_slice = util_alloc_matrix(row_count, col_count);
  int** thresholded_slice = util_alloc_matrix(row_count, col_count);

  // Starting smoothing process. After smoothing each point, slave is checking whether
  // there is a message from other slaves
  int start_x, start_y;                   // Starting point of slice processing
  int end_x, end_y;                       // Ending point of slice processing (exclusive)
  int stage = STAGE_SMOOTHING;
  util_decide_starting_position(slice_type, row_count, col_count, stage, &start_x, &start_y, &end_x, &end_y);

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

    if(message_exists) {
      int message_size, message_data, message_source;
      MPI_Get_count(&status, MPI_INT, &message_size);
      MPI_Recv(&message_data, message_size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      message_source = status.MPI_SOURCE;

      if(status.MPI_TAG == FINISH_SMOOTHING_TAG) {
        debug_1("Freeing slice matrix", &rank);
        for(int row = 0; row < row_count; row++) {
           free(*(slice_matrix + row)); 
        }
        free(slice_matrix);
        debug_1("Finished smoothing. Waiting until further information.", &rank);
        job_finished = true;
        continue;
      } else if(status.MPI_TAG == START_THRESHOLDING_TAG) {
        debug_1("I am starting thresholding...", &rank);
        stage = STAGE_THRESHOLDING;
        util_decide_starting_position(slice_type, row_count, col_count, stage, &start_x, &start_y, &end_x, &end_y);
        curr_x = start_x;
        curr_y = start_y;
        job_finished = false; // Work work work work work
        continue;
      } else if(status.MPI_TAG == FINISH_THRESHOLDING_TAG) {
        debug_1("Finished thresholding. Returning...", &rank);
        return;
      } else if(someone_need_me) {
        int x_index = message_data;
        int demander_source = message_source;

        int* points = util_prepare_points_for_demander(slice_matrix, x_index, status.MPI_TAG, end_y);
        /* Sending point data */
        MPI_Send(points, 3, MPI_INT, demander_source, (50 + demander_source), MPI_COMM_WORLD);
        free(points);
      } else {
        debug_1("Received an unidentified message, there is something wrong!", &rank);
        exit(0);
      }
    } else { // Do your own job
      if(job_finished) continue;
      if(stage == STAGE_SMOOTHING) {
        /*
         * Checking boundary conditions where we need to rearrange position
         */
        if(curr_x == end_x) {
          if(curr_y == end_y - 1) {
            /* Informing master that my job is done */
            int slice_size = row_count * col_count;
            MPI_Send(&slice_size, 1, MPI_INT, 0, SMOOTHING_DONE_TAG, MPI_COMM_WORLD);
            int* slice = serialize_slice(smoothened_slice, row_count, col_count);
            MPI_Send(slice, row_count * col_count, MPI_INT, 0, FOLLOWING_SMOOTHING_DONE_TAG, MPI_COMM_WORLD);
            job_finished = true;
          } else {
            curr_x = 1;
            curr_y++;
          }
        } else {  // Do your own job

          bool is_low_row = curr_y == end_y - 1;
          bool is_high_row = curr_y == start_y;
          int total;

          int special_row = util_determine_special_row(slice_type, is_low_row, is_high_row);

          process_rows_for_smoothing(slice_matrix, curr_x, curr_y, special_row,
                                     &is_demanded, &should_continue, &total, &rank);

          if(should_continue) {
            continue;
          }
          *(*(smoothened_slice + curr_x) + curr_y) = total;
          curr_x++;
        }
      } else if(stage == STAGE_THRESHOLDING) {
        /* debug_4("Processing (%d, %d), end_x = %d", &curr_x, &curr_y, &end_x, &rank); */
        if(curr_x == end_x) {
          if(curr_y == end_y - 1) {
            job_finished = true;
            /* Informing master that my job is done */
            int slice_size = row_count * col_count;
            MPI_Send(&slice_size, 1, MPI_INT, 0, THRESHOLDING_DONE_TAG, MPI_COMM_WORLD);
            int* slice = serialize_slice(thresholded_slice, row_count, col_count);
            MPI_Send(slice, row_count * col_count, MPI_INT, 0, FOLLOWING_THRESHOLDING_DONE_TAG, MPI_COMM_WORLD);
          } else {
            curr_x = 2;
            curr_y++;
          }
        } else {
          /* debug_3("Working on thresholding: (%d, %d)", &curr_x, &curr_y, &rank); */
          int total = 120;
          *(*(thresholded_slice + curr_x) + curr_y) = total;

          curr_x++;
        }
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
    /* printf("Master is finished.\n"); */
  } else {
    slave();
    printf("Slave is finished.\n");
  }

  printf("Finalizing %d\n", rank);
  MPI_Finalize();
}
