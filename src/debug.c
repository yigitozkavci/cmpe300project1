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

void debug_7(char *message, int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, int *rank) {
  MPI_Send(message, strlen(message), MPI_CHAR, 0, DEBUG_MESSAGE_4_TAG, MPI_COMM_WORLD);
  MPI_Send(arg1, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
  MPI_Send(arg2, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
  MPI_Send(arg3, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
  MPI_Send(arg4, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
  MPI_Send(arg5, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
  MPI_Send(arg6, 1, MPI_INT, 0, DEBUG_MESSAGE_FOLLOWUP_TAG, MPI_COMM_WORLD);
}
