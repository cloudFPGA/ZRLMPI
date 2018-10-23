
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mpi.h"
#include "jacobi_common.h"


int main( int argc, char **argv )
{
  int        rank, size;
  MPI_Status status;

  MPI_Init( &argc, &argv );

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );

  if(size%2 != 1)
  {//for now, we need uneven processes
    printf("ERROR: only uneven numbers of processes are supported!\n"); 
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  if(rank == 0)
  {// Master ...

#ifdef USE_INTEGER
    int grid[DIM][DIM];
#else
    double grid[DIM][DIM];
#endif

    //fill with zeros and init borders with 1
    for(int i = 0; i<DIM; i++)
    {
      for(int j = 0; j<DIM; j++)
      {

        if( i == 0 || i == DIM-1 || j == 0 || j == DIM-1)
        {
#ifdef USE_INTEGER
          grid[i][j] = 4;
#else
          grid[i][j] = 1;
#endif
        }
        else {
          grid[i][j] = 0;
        }
      }
    }

#ifdef USE_INTEGER
    print_array((const int*) grid, DIM, DIM);
#else 
    print_array((const double*) grid, DIM, DIM);
#endif

    printf("Ditstribute data and start client nodes.\n");

    MPI_Barrier(MPI_COMM_WORLD);

    //FOR DEBUG ONLY
    if(size != 3)
    {//assert...
      printf("size != 3\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for(int i = 1; i<size; i++)
    {
      //distribute data 
      int first_send_line = 0; 
      if(i == size-1)
      {
        first_send_line = (LDIMY-2);
      }
#ifdef USE_INTEGER
      MPI_Send(&grid[first_send_line][0], LDIMX*LDIMY, MPI_INTEGER, i, DATA_CHANNEL_TAG, MPI_COMM_WORLD);
#else
      MPI_Send(&grid[first_send_line][0], LDIMX*LDIMY, MPI_DOUBLE, i, DATA_CHANNEL_TAG, MPI_COMM_WORLD);
#endif 

    }

#ifdef USE_INTEGER
    int tmp[LDIMY][LDIMX]; 
#else
    double tmp[LDIMY][LDIMX]; 
#endif 

    for(int i = 1; i<size; i++)
    {
      //collect results 
#ifdef USE_INTEGER
      MPI_Recv(tmp, LDIMY*LDIMX, MPI_INTEGER, i, DATA_CHANNEL_TAG, MPI_COMM_WORLD, &status); 
#else
      MPI_Recv(tmp, LDIMY*LDIMX, MPI_DOUBLE, i, DATA_CHANNEL_TAG, MPI_COMM_WORLD, &status); 
#endif

      int first_target_line = 0;
      if(i == size-1)
      {
        first_target_line = LDIMY -2;
      }

      for(int y = 1; y < LDIMY-1; y++)
      {
        for(int x = 1; x < LDIMX-1; x++)
        {
          grid[first_target_line + y][x] = tmp[y][x];
        }
      }

    }

#ifdef USE_INTEGER
    print_array((const int*) grid, DIM, DIM);
#else 
    print_array((const double*) grid, DIM, DIM);
#endif

    MPI_Barrier(MPI_COMM_WORLD);

    printf("Done.\n"); 

  } else { 
    //Slaves ... 

    printf("Here is rank %d, size is %d. \n",rank, size);
    MPI_Barrier(MPI_COMM_WORLD);

#ifdef USE_INTEGER
    int local_grid[LDIMY][LDIMX];
    int local_new[LDIMY][LDIMX];

    MPI_Recv(local_grid, LDIMY*LDIMX, MPI_INTEGER, 0, DATA_CHANNEL_TAG, MPI_COMM_WORLD, &status);
#else 
    double local_grid[LDIMY][LDIMX];
    double local_new[LDIMY][LDIMX];

    MPI_Recv(local_grid, LDIMY*LDIMX, MPI_DOUBLE, 0, DATA_CHANNEL_TAG, MPI_COMM_WORLD, &status);
#endif

    //print_array((const double*) local_grid, LDIMX, LDIMY);

    //only one iteration for now
    //treat all borders equal, the additional lines in the middle are cut out from the merge at the server
    for(int i = 1; i<LDIMY-1; i++)
    {
      for(int j = 1; j<LDIMX-1; j++)
      {
        local_new[i][j] = (local_grid[i][j-1] + local_grid[i][j+1] + local_grid[i-1][j] + local_grid[i+1][j]) / 4.0;
      }
    }
#ifdef USE_INTEGER
    MPI_Send(local_new, LDIMY*LDIMX, MPI_INTEGER, 0, DATA_CHANNEL_TAG, MPI_COMM_WORLD);
#else 
    MPI_Send(local_new, LDIMY*LDIMX, MPI_DOUBLE, 0, DATA_CHANNEL_TAG, MPI_COMM_WORLD);
#endif

    //print_array((const double*) local_new, LDIMX, LDIMY);

    MPI_Barrier(MPI_COMM_WORLD);

    printf("Calculation finished.\n");

  }


  MPI_Finalize();
  return 0;
}


