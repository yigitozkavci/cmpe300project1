#include "util.h"

void util_decide_starting_position(
  int slice_type,
  int row_count,
  int col_count,
  int stage,
  int* start_x,
  int* start_y,
  int* end_x,
  int* end_y
) {
  if(stage == STAGE_SMOOTHING) {
    *start_x = 1;
    *end_x = col_count - 1;
    if(slice_type == SLICE_TYPE_MIDDLE) {
      *start_y = 0;
      *end_y = row_count;
    } else if(slice_type == SLICE_TYPE_TOP) {
      *start_y = 1;
      *end_y = row_count;
    } else if(slice_type == SLICE_TYPE_BOTTOM) {
      *start_y = 0;
      *end_y = row_count - 1;
    } else {
      printf("Wrong slice type: %d", slice_type);
      exit(0);
    }
  } else if(stage == STAGE_THRESHOLDING) {
    *start_x = 2;
    *end_x = col_count - 2;
    if(slice_type == SLICE_TYPE_MIDDLE) {
      *start_y = 0;
      *end_y = row_count;
    } else if(slice_type == SLICE_TYPE_TOP) {
      *start_y = 2;
      *end_y = row_count;
    } else if(slice_type == SLICE_TYPE_BOTTOM) {
      *start_y = 0;
      *end_y = row_count - 1;
    } else {
      printf("Wrong slice type: %d", slice_type);
      exit(0);
    }
  } else {
    printf("Wrong stage type: %d", stage);
  }
}

int** util_alloc_matrix(int row_count, int col_count) {
  // Allocating space for our smoothened_slice
  int** result = (int**)malloc(col_count * sizeof(int*));
  for(int col = 0; col < col_count; col++) {
    *(result + col) = (int*)malloc(row_count * sizeof(int));
    for(int row = 0; row < row_count; row++) {
      *(*(result + col) + row) = 0;
    }
  }
  return result;
}

int* util_prepare_points_for_demander(int** slice_matrix, int x_index, int tag, int end_y) {
  int y_index;
  /*
   * Determining y-index based on demand type of data.
   * Slaves can demand data from either higher or lower rows.
   */
  if(tag == DEMAND_DATA_FROM_UPPER_SLICE_TAG) {
    y_index = end_y - 1;
  } else if(tag == DEMAND_DATA_FROM_LOWER_SLICE_TAG) {
    y_index = 0;
  } else {
    printf("[!] Wrong tag: %d\n", tag);
    exit(0);
  }

  /* Writing point data to send */
  int *points = malloc(sizeof(int) * 3);
  for(int i = x_index - 1; i <= x_index + 1; i++) {
    *(points + i - x_index + 1) = *(*(slice_matrix + y_index) + i);
  }

  return points;
}

int util_determine_special_row(int slice_type, bool is_low_row, bool is_high_row) {
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

  return special_row;
}
