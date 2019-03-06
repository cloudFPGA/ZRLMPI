
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "MPI.hpp"
#include "test.hpp"

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

int app_main()
{
  int        rank, size;
  MPI_Status status;

  MPI_Init();

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );


  printf("Here is rank %d, size is %d. \n",rank, size);

  if(rank == 0)
  {// Master ...
    int grid[DIM][DIM];

    //fill with zeros and init borders with 1
    for(int i = 0; i<DIM; i++)
    {
      for(int j = 0; j<DIM; j++)
      {

        if( i == 0 || i == DIM-1 || j == 0 || j == DIM-1)
        {
          grid[i][j] = CORNER_VALUE_INT;
        }
        else {
          grid[i][j] = 0;
        }
      }
    }

    print_array((const int*) grid, DIM, DIM);

    printf("Ditstribute data and start client nodes.\n");


    for(int i = 1; i<size; i++)
    {
      //distribute data ONLY in PACKETLENGTH packets
      int first_send_line = (i-1)*LDIMY;
      int last_send_line = i*LDIMY + 1;
      if(i == size-1)
      {
        //first_send_line = (LDIMY-2);
        last_send_line--;
      }
      for(int j = first_send_line; j<last_send_line; j++)
      {
        printf("Sending line %d to rank %d.\n", j, i);
        MPI_Send(&grid[j][0], PACKETLENGTH, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
      }
    }

    int tmp[LDIMY+1][LDIMX];

    for(int i = 1; i<size; i++)
    {
      //collect results
      int number_of_recv_packets = LDIMY + 1; 
      if(i == size -1)
      {
        number_of_recv_packets--;
      }
      for(int j = 0; j< number_of_recv_packets; j++)
      {
        MPI_Recv(&tmp[j][0], PACKETLENGTH, MPI_INTEGER, i, 0, MPI_COMM_WORLD, &status);
      }

      int first_recv_line = (i-1)*LDIMY;
      //int last_recv_line = i*LDIMY + 1;
      //if(i == size-1)
      //{
      //  //first_send_line = (LDIMY-2);
      //  last_recv_line--;
      //}

      for(int y = 0; y < number_of_recv_packets; y++)
      {
        if(y == 0 && i == 1)
        {
          continue;
        }
        if( y == number_of_recv_packets - 1 && i == size - 1)
        {
          continue;
        }
        for(int x = 1; x < LDIMX-1; x++)
        {
          grid[first_recv_line + y][x] = tmp[y][x];
        }
      }

    }

    printf("Done.\n");
    print_array((const int*) grid, DIM, DIM);


  } else { 
    //Slaves ... 

    int local_grid[LDIMY + 1][LDIMX];
    int local_new[LDIMY + 1][LDIMX];

    //MPI_Recv(&local_grid[0][0], LDIMY*LDIMX, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
    int number_of_recv_packets = LDIMY + 1; 
    if(rank == size -1)
    {
      number_of_recv_packets--;
    }

    for(int j = 0; j< LDIMY + 1; j++)
    {
      if(j == number_of_recv_packets)
      {
        break;
      }
      MPI_Recv(&local_grid[j][0], PACKETLENGTH, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
    }

    // print_int_array((const int*) local_grid, LDIMX, LDIMY);

    //only one iteration for now
    //treat all borders equal, the additional lines in the middle are cut out from the merge at the server
    for(int i = 1; i < LDIMY + 1; i++)
    {
      if(i == number_of_recv_packets)
      {
        break;
      }
      for(int j = 1; j<LDIMX-1; j++)
      {
        local_new[i][j] = (local_grid[i][j-1] + local_grid[i][j+1] + local_grid[i-1][j] + local_grid[i+1][j]) / 4.0;
      }
    }
    //MPI_Send(&local_new[0][0], LDIMY*LDIMX, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);
    for(int j = 0; j< LDIMY + 1; j++)
    {
      if(j == number_of_recv_packets)
      {
        break;
      }
      MPI_Send(&local_new[j][0], PACKETLENGTH, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);
    }

    //print_int_array((const int*) local_new, LDIMX, LDIMY);

    //printf("Calculation finished.\n");
  }

  MPI_Finalize();
  return 0;
}


