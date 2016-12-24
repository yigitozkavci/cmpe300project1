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

#include "debug.h"
#include "mpi.h"
#include <string.h>
#include "tag_types.h"

void debug_1(char *message, int *rank) {
  MPI_Send(message, strlen(message), MPI_CHAR, 0, DEBUG_MESSAGE_1_TAG, MPI_COMM_WORLD);
}

void debug_2(char *message, int *arg1, int *rank) {
  MPI_Send(message, strlen(message), MPI_CHAR, 0, DEBUG_MESSAGE_2_TAG, MPI_COMM_WORLD);
  MPI_Send(arg1, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
}

void debug_3(char *message, int *arg1, int *arg2, int *rank) {
  MPI_Send(message, strlen(message), MPI_CHAR, 0, DEBUG_MESSAGE_3_TAG, MPI_COMM_WORLD);
  MPI_Send(arg1, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
  MPI_Send(arg2, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
}

void debug_4(char *message, int *arg1, int *arg2, int *arg3, int *rank) {
  MPI_Send(message, strlen(message), MPI_CHAR, 0, DEBUG_MESSAGE_4_TAG, MPI_COMM_WORLD);
  MPI_Send(arg1, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
  MPI_Send(arg2, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
  MPI_Send(arg3, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
}
