
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
//#include "mpi.h"
#include "ZRLMPI.hpp"
#include "pingpong.h"
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
  if(size != 2)
  {//for now, we need uneven processes
    printf("ERROR: only a culster size of two is supported!\n"); 
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  printf("Here is rank %d, size is %d. \n",rank, size);
#endif

#ifdef DEBUG
  timestamp_t t0 = get_timestamp();
#endif

  int msg[MSG_SIZE];
  for(int i = 0; i < ITERATIONS; i++) {
    if(rank == 0) {
      msg[0] = 0xcaffee;
      MPI_Send(&msg[0], MSG_SIZE, MPI_INTEGER, 1, 0, MPI_COMM_WORLD);
      MPI_Recv(&msg[0], MSG_SIZE, MPI_INTEGER, 1, 0, MPI_COMM_WORLD, &status);
      printf("[rank %d, iter %d] received msg 0x%04x\n", rank, i, msg[0]);
    } else { 
      MPI_Recv(&msg[0], MSG_SIZE, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
      printf("[rank %d, iter %d] received msg 0x%04x\n", rank, i, msg[0]);
      MPI_Send(&msg[0], MSG_SIZE, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);
    }
  }


#ifdef DEBUG 
  if(rank == 0)
  {
    timestamp_t t1 = get_timestamp();
    double secs = (t1 - t0) / 1000000.0L;
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\tMPI verify runtime: %lfs\n", secs);
  }
#endif

  MPI_Finalize();
  return 0;
}


