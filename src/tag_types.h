/* Student Name: Yiğit Özkavcı
 * Student Number: 2013400111
 * Compile Status: Compiling
 * Program Status: Working
 *
 * Notes:
 * You can compile the code on any unix/linux platform with typing:
 * $ make
 *
 * Then run with:
 * $ mpiexec -n <n_of_processors> <program> <in_file> <out_file> <threshold>
 */

#ifndef TAG_TYPES_H_
#define TAG_TYPES_H_

#define DIETAG 1
#define SLICE_TAG 2
#define SLICE_SIZE_TAG 3
#define SLICE_TYPE_TAG 4
#define DEMAND_DATA_FROM_UPPER_SLICE_TAG 5
#define DEMAND_DATA_FROM_LOWER_SLICE_TAG 6
#define POINT_DATA_TAG 7
#define DEBUG_MESSAGE_1_TAG 8
#define DEBUG_MESSAGE_2_TAG 9
#define DEBUG_MESSAGE_3_TAG 10
#define DEBUG_MESSAGE_4_TAG 11
#define DEBUG_MESSAGE_FOLLOWUP_TAG 12
#define SMOOTHING_DONE_TAG 13
#define FOLLOWING_SMOOTHING_DONE_TAG 14
#define THRESHOLDING_DONE_TAG 15
#define FOLLOWING_THRESHOLDING_DONE_TAG 16
#define FINISH_SMOOTHING_TAG 17
#define START_THRESHOLDING_TAG 18
#define FINISH_THRESHOLDING_TAG 19

#endif
