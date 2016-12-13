#ifndef UTIL_H_
#define UTIL_H_

#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include "tag_types.h"
#include <stdbool.h>


/* There are 3 types of slices: */
#define SLICE_TYPE_TOP 1    /* Meaning slice is at the very top. */
#define SLICE_TYPE_MIDDLE 2 /* Meaning slice is neither at the top or bottom. */
#define SLICE_TYPE_BOTTOM 3 /* Meaning slice is at the very bottom. */

/* There are 2 stages to work on for each slave: */
#define STAGE_SMOOTHING 1
#define STAGE_THRESHOLDING 2

/**********************************************************************
 * Given slice type and dimensions, decides what start and
 * end points of matrix we are going to work on.
 **********************************************************************/
void util_decide_starting_position(
  int slice_type, /* Type of the slice. It can take 3 values, which are defined above this file. */
  int row_count,      /* Row count of slice matrix. */
  int col_count,      /* Column count of slice matrix. */ 
  int stage,          /* Stage can be whether SMOOTHING or THRESHOLDING. */
  int is_slice_alone, /* Stage can be whether SMOOTHING or THRESHOLDING. */
  int* start_x,       /* X dimension of starting point. */
  int* start_y,       /* Y dimension of starting point. */
  int* end_x,         /* X dimension of ending point. */
  int* end_y          /* Y dimension of ending point. */
);

/**********************************************************************
 * Given dimensions, allocates a matrix.
 **********************************************************************/
int** util_alloc_matrix(
  int row_count, /* Row count of matrix to return. */
  int col_count  /* Row count of matrix to return. */
);


/**********************************************************************
 * When any other slice demands data of 3 points from you, you simply
 * give them. This method gives information of 3 points of its slice
 * matrix: (x_index - 1, x_index, x_index + 1).
 *
 * Tag is used to determine whether information is demanded from top
 * or bottom. If bottom, this method also needs to know end_y.
 **********************************************************************/
int* util_prepare_points_for_demander(int** slice_matrix, int x_index, int tag, int end_y);

/**********************************************************************
 * Special row is the row of which took data from another slice.
 *
 * Think of a slice;
 * if row is the bottom row of this particular slice, it's called the low row.
 * else if it's the top row, it's high row.
 * Else, it's neither of them.
 *
 * We need this because... well, reasons. Read the implementation.
 **********************************************************************/
int util_determine_special_row(int slice_type, bool is_low_row, bool is_high_row);

#endif
