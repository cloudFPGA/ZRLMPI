/*******************************************************************************
 * Copyright 2018 -- 2023 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mpi.h"
#include "triangle.h"


#ifdef DEBUG
#include <sys/time.h>
typedef unsigned long long timestamp_t;

static timestamp_t get_timestamp ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  return  now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}
#endif

int main( int argc, char **argv )
{
  int        rank, size;
  MPI_Status status;

  MPI_Init( &argc, &argv );

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );

#ifdef DEBUG
  if(size != 3)
  {//for now, we need uneven processes
    printf("ERROR: only a culster size of three is supported!\n"); 
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  printf("Here is rank %d, size is %d. \n",rank, size);
#endif

#ifdef DEBUG
  timestamp_t t0 = get_timestamp();
#endif

  int msg[MSG_SIZE];
  int next_node = (rank + 1) % size;
  int previous_node = rank -1;

  for(int i = 0; i < ITERATIONS; i++) {
    if(rank == 0) {
      msg[0] = 0xcaffee;
      MPI_Send(&msg[0], 1, MPI_INTEGER, 1, 0, MPI_COMM_WORLD);
      MPI_Recv(&msg[0], 1, MPI_INTEGER, size-1, 0, MPI_COMM_WORLD, &status);
      printf("[rank %d, iter %d] received message 0x%04x\n", rank, i, msg[0]);
    } else {
      MPI_Recv(&msg[0], 1, MPI_INTEGER, previous_node, 0, MPI_COMM_WORLD, &status);
      printf("[rank %d, iter %d] received message 0x%04x\n", rank, i, msg[0]);
      MPI_Send(&msg[0], 1, MPI_INTEGER, next_node, 0, MPI_COMM_WORLD);
    }
  }


#ifdef DEBUG 
  if(rank == 0)
  {
    timestamp_t t1 = get_timestamp();
    double secs = (t1 - t0) / 1000000.0L;
    printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>\tMPI verify runtime: %lfs\n", secs);
  }
#endif

  MPI_Finalize();
  return 0;
}


