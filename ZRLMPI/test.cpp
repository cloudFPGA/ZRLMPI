
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "test.hpp"
#include "MPI.hpp"


//for debugging
/*
   void print_int_array(const int *A, size_t width, size_t height)
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
   */


//int main( int argc, char **argv )
//DUE TO SHITTY HLS...
void app_main()
{
  MPI_Init();
  //MPI_Init(&argc, &argv);


  int        rank, size;
  MPI_Status status;


  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );

  //Slaves ...

  printf("Here is rank %d, size is %d. \n",rank, size);

	    int grid[DIM][DIM];

	    //fill with zeros and init borders with 1
	    for(int i = 0; i<DIM; i++)
	    {
	      for(int j = 0; j<DIM; j++)
	      {

	        if( i == 0 || i == DIM-1 || j == 0 || j == DIM-1)
	        {
	          grid[i][j] = 4;
	        }
	        else {
	          grid[i][j] = 0;
	        }
	      }
	    }
	
      //    print_array((const int*) grid, DIM, DIM);


	    printf("Ditstribute data and start client nodes.\n");


	    for(int i = 1; i<size; i++)
	    {
	      //distribute data
	      int first_send_line = 0;
	      if(i == size-1)
	      {
	        first_send_line = (LDIMY-2);
	      }
	      MPI_Send(&grid[first_send_line][0], LDIMX*LDIMY, MPI_INTEGER, i, DATA_CHANNEL_TAG, MPI_COMM_WORLD);
	    }

	    int tmp[LDIMY][LDIMX];

	    for(int i = 1; i<size; i++)
	    {
	      //collect results
	      MPI_Recv(tmp, LDIMY*LDIMX, MPI_INTEGER, i, DATA_CHANNEL_TAG, MPI_COMM_WORLD, &status);

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

	    printf("Done.\n");

      MPI_Finalize();

  return;
}



