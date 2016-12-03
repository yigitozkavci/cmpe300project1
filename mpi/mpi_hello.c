/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
 *
 * Sample MPI "hello world" application in C
 */

#include <stdio.h>
#include "mpi.h"

int main(int argc, char* argv[])
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int send_data = 100, receive_data;
    MPI_Status status;
    if(rank == 0) {
      printf("Starting process: %d\n", 0);
      MPI_Send(&send_data, 1, MPI_INT, 1, 7, MPI_COMM_WORLD);
    } else if(rank == 1) {
      printf("Starting process: %d\n", 1);
      MPI_Recv(&receive_data, 1, MPI_INT, 0, 7, MPI_COMM_WORLD, &status);
      printf("Received: %d\n", receive_data);
    } else {
      printf("Starting process: %d\n", rank);
    }
    MPI_Finalize();

    return 0;
}
