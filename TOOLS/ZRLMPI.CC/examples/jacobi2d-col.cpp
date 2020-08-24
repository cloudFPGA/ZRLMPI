
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mpi.h"
#include "jacobi2d-col.hpp"
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

#ifdef ZRLMPI_SW_ONLY
void print_array(const int *A, size_t width, size_t height)
{
  printf("\n");
  for(size_t i = 0; i < height; ++i)
  {
    for(size_t j = 0; j < width; ++j)
    {
      printf("%d ", A[i * width + j]);
    }
    printf("\n");
  }
  printf("\n");
}
#endif

int main( int argc, char **argv )
{
  int        rank, size;
  MPI_Status status;
  int iterations[1];
  int start_line, end_line, result_start_line;

  MPI_Init( &argc, &argv );

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );

#ifdef DEBUG
  if(DIM%size!=0)
  {//for now, we need uneven processes
    printf("ERROR: Dimension could not be split evenely (this is a simple demo)!\n"); 
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  timestamp_t t0, t1;
#endif

  int grid[DIM][DIM];

  printf("Here is rank %d, size is %d. \n",rank, size);

#ifdef DEBUG
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  if(rank == 0)
  {// Master ...
#ifdef DEBUG
    t0 = get_timestamp();
#endif
    //fill with zeros and init borders with 1
    for(int i = 0; i<DIM; i++)
    {
      for(int j = 0; j<DIM; j++)
      {

        if( i == 0 || i == DIM - 1 || j == 0 || j == DIM-1)
          //if( i == 1 || i == DIM || j == 0 || j == DIM-1)
        {
          grid[i][j] = CORNER_VALUE_INT;
        }
        else {
          grid[i][j] = 0;
        }
      }
    }

    print_array((const int*) &grid[0][0], DIM, DIM);

    printf("Distribute number of iterations: %d.\n", ITERATIONS);
    iterations[0] = ITERATIONS;
    for(int i = 1; i<size; i++)
    {
      MPI_Send(&iterations[0], 1, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
    }

    printf("Ditstribute data and start client nodes.\n");

  } //if master

  //worker code

  //int local_grid[DIM][DIM];
  int local_new[DIM][DIM];
  int local_dim = DIM/size;

  if(rank != 0)
  { //receive iterations
    printf("Waiting for iterations...\n");
    MPI_Recv(&iterations[0], 1, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
    printf("...we are doing %d iterations.\n",iterations[0]);
  }

  start_line = rank * local_dim;
  end_line = (rank+1)*local_dim;
  result_start_line = start_line;
  //for border regions
  int border_startline = start_line;
  int border_endline = end_line;
  if(rank != 0)
  {
    border_startline--;
  }
  if(rank == (size -1))
  {
    border_endline--;
  }

#ifdef DEBUG
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  //for(int l = 0; l < ITERATIONS; l++)
  for(int l = 0; l < iterations[0]; l++)
  {
    //main data junk
    //copies [start_line;end_line[ !!
    MPI_Scatter(&grid[0][0], local_dim*DIM, MPI_INTEGER, &grid[start_line][0], local_dim*DIM, MPI_INTEGER, 0, MPI_COMM_WORLD);
    //int *new_start = &grid[0][0] + l*5;
    //internal border regions
    if(rank==0)
    {
      for(int r = 1; r < size; r++)
      {
        int first_line = r*local_dim;
        int last_line = (r+1)*local_dim -1;
        if(r != 0)
        {
          first_line--;
        }
        if(r != size-1)
        {
          last_line++;
        }
        MPI_Send(&grid[first_line][0], DIM, MPI_INTEGER, r, 0, MPI_COMM_WORLD);
        MPI_Send(&grid[last_line][0], DIM, MPI_INTEGER, r, 0, MPI_COMM_WORLD);
      }
      //take care of own data
      //for(int i = 0; i < DIM; i++)
      //{
      //  local_grid[end_line][i] = grid[end_line][i];
      //}
    } else {
      MPI_Recv(&grid[border_startline][0], DIM, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
      MPI_Recv(&grid[border_endline][0], DIM, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
    }

    //#ifdef DEBUG
    //if(rank == size-1)
    //{
    //    print_array((const int*) local_grid, DIM, DIM);
    //}
    //#endif

    //treat all borders equal, the additional lines in the middle aren't send back
    for(int i = start_line; i < end_line; i++)
    {
      for(int j = 0; j< DIM; j++)
      {
        if(i == 0 || i == DIM-1 || j == 0 || j == DIM -1)
        {
          local_new[i][j] = grid[i][j];
        } else {
          local_new[i][j] = (grid[i][j-1] + grid[i][j+1] + grid[i-1][j] + grid[i+1][j]) >> 2;
        }
      }
    }

    //we don't copy the borders in all cases
    MPI_Gather(&local_new[result_start_line][0], local_dim*DIM, MPI_INTEGER, &grid[0][0], local_dim*DIM, MPI_INTEGER, 0, MPI_COMM_WORLD);
#ifdef DEBUG
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 0)
    {
      print_array((const int*) grid, DIM, DIM);
    }
#endif
  }

  if(rank==0)
  {
#ifdef DEBUG 
    t1 = get_timestamp();
    double secs = (t1 - t0) / 1000000.0L;
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\tMPI verify runtime: %lfs\n", secs);
#endif

    printf("\nDone.\n");
    print_array((const int*) &grid[0][0], DIM, DIM);
  }
  MPI_Finalize();

  return 0;
}


