#include <stdio.h>
#include "mpi.h"
#define WORKTAG 1
#define DIETAG 2

void master_io() {
  printf("I am MASTER!\n");
  int size, rank;
  MPI_Status status;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  for(rank = 1; rank < size; ++rank) {
    int work = 2;
    if(rank % 3 == 0) {
      MPI_Send(&work, 1, MPI_INT, rank, DIETAG, MPI_COMM_WORLD);
    } else {
      MPI_Send(&work, 1, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);
    }
  }
}

void slave_io() {
  MPI_Status status;
  int work, rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  for(;;) {
    MPI_Recv(&work, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    if(status.MPI_TAG == DIETAG) {
      printf("I dieeded\n");
      return;
    } else if(status.MPI_TAG == WORKTAG) {
      printf("I am working\n");
    }
  }
}

int main(int argc, char* argv[])
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(rank == 0) {
      master_io();
    } else {
      slave_io();
    }
    MPI_Finalize();

    return 0;
}
