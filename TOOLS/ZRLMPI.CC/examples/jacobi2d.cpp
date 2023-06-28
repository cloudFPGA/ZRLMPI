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
#include "jacobi2d.hpp"
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
  iterations[0] = ITERATIONS;

  MPI_Init( &argc, &argv );

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );

#ifdef DEBUG
  if(size%2 != 1)
  {//for now, we need uneven processes
    printf("ERROR: only uneven numbers of processes are supported!\n"); 
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
#endif

  //int worker_nodes = size - 1; //root is not planning to do smth;

  printf("Here is rank %d, size is %d. \n",rank, size);

  if(rank == 0)
  {// Master ...
#ifdef DEBUG
    timestamp_t t0 = get_timestamp();
#endif
    int grid[DIM+2][DIM]; //create an array with two lines more

    //fill with zeros and init borders with 1
    //for(int i = 0; i<DIM; i++)
    for(int i = 0; i<DIM+2; i++)
    {
      for(int j = 0; j<DIM; j++)
      {

        //if( i == 0 || i == DIMi - 1 || j == 0 || j == DIM-1)
        if( i == 1 || i == DIM || j == 0 || j == DIM-1)
        {
          grid[i][j] = CORNER_VALUE_INT;
        }
        else {
          grid[i][j] = 0;
        }
      }
    }

    //print_array((const int*) &grid[1][0], DIM, DIM);

    printf("Distribute number of iterations: %d.\n", ITERATIONS);
    iterations[0] = ITERATIONS;
    for(int i = 1; i<size; i++)
    {
        MPI_Send(&iterations[0], 1, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
    }

    printf("Ditstribute data and start client nodes.\n");

    for(int l = 0; l < ITERATIONS; l++)
    {
      for(int i = 1; i<size; i++)
      {
        int first_send_line = (i-1)*WORKER_LINES - 1;
        int last_send_line = i*WORKER_LINES  + 1;
        //if(i == 1)
        //{
        //  first_send_line++;
        //}
        //if(i == size-1)
        //{
        //  last_send_line--;
        //}
        int size_of_this_message = last_send_line - first_send_line;
        if(l == 0)
        {
          printf("Sending %d lines of data to rank %d at start line %d.\n", size_of_this_message, i, first_send_line);
        }
          //MPI_Send(&grid[1 + first_send_line][0], size_of_this_message*DIM, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
        for(int j = 0; j<size_of_this_message; j++)
        {
          //MPI_Send(&grid[1 + first_send_line + j][0], DIM, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
          for(int k = 0; k < PACKETS_PER_LINE; k++)
          {
            MPI_Send(&grid[1 + first_send_line + j][k*PACKETLENGTH], PACKETLENGTH, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
          }
        }
      }

    printf("Start collecting results...\n");
      int tmp[WORKER_LINES+2][DIM];

      for(int i = 1; i<size; i++)
      {
        //collect results
        int number_of_recv_lines = WORKER_LINES + 2;
        //if(i == size -1 || i == 1)
        //{
        //  number_of_recv_lines--;
        //}
        //MPI_Recv((int*) &tmp, number_of_recv_lines*DIM, MPI_INTEGER, i, 0, MPI_COMM_WORLD, &status);
        for(int j = 0; j < number_of_recv_lines; j++)
        {
            //MPI_Recv(&tmp[j][0], DIM, MPI_INTEGER, i, 0, MPI_COMM_WORLD, &status);
          for(int k = 0; k < PACKETS_PER_LINE; k++)
          {
            MPI_Recv(&tmp[j][k*PACKETLENGTH], PACKETLENGTH, MPI_INTEGER, i, 0, MPI_COMM_WORLD, &status);
          }
        }

        // print_array((const int*) tmp, DIM, WORKER_LINES+1);

        int first_recv_line = (i-1)*WORKER_LINES;
        int line_skip = 0;
        if(i == 1)
        {
          line_skip++;
        }
        int max_line_use = WORKER_LINES + 1;
        if(i == size -1)
        {
          max_line_use--;
        }

        for(int y = 1 + line_skip; y < max_line_use; y++)
        {
          for(int x = 1; x < DIM-1; x++)
          {
            grid[first_recv_line + y][x] = tmp[y][x];
          }
        }

      }
      printf("\nFinished iteration %d of %d.\n",l+1,ITERATIONS);
      //print_array((const int*) &grid[1][0], DIM, DIM);
    }

    printf("\nDone.\n");
    //print_array((const int*) &grid[1][0], DIM, DIM);

#ifdef DEBUG 
    timestamp_t t1 = get_timestamp();
    double secs = (t1 - t0) / 1000000.0L;
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\tMPI verify runtime: %lfs\n", secs);
#endif

  } else { 
    //Slaves ... 

    int local_grid[WORKER_LINES + 1][DIM];
    int local_new[WORKER_LINES  + 1][DIM];

    MPI_Recv(&iterations[0], 1, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
    int number_of_recv_lines = WORKER_LINES + 2;
    //if(rank == size -1 || rank == 1)
    //{
    //  number_of_recv_lines--;
    //}
    //for(int l = 0; l < ITERATIONS; l++)
    for(int l = 0; l < iterations[0]; l++)
    {
        //MPI_Recv(&local_grid[0][0], number_of_recv_lines*DIM, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
      for(int j = 0; j < number_of_recv_lines; j++)
      {
        //MPI_Recv(&local_grid[j][0], DIM, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
        for(int k = 0; k < PACKETS_PER_LINE; k++)
        {
          MPI_Recv(&local_grid[j][k*PACKETLENGTH], PACKETLENGTH, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);
        }
      }

      //#ifdef DEBUG
      //    print_array((const int*) local_grid, DIM, WORKER_LINES);
      //#endif

      //treat all borders equal, the additional lines in the middle are cut out from the merge at the server
      for(int i = 1; i < WORKER_LINES + 1; i++)
      {
        for(int j = 1; j<DIM-1; j++)
        {
          //local_new[i][j] = (local_grid[i][j-1] + local_grid[i][j+1] + local_grid[i-1][j] + local_grid[i+1][j]) / 4.0;
          local_new[i][j] = (local_grid[i][j-1] + local_grid[i][j+1] + local_grid[i-1][j] + local_grid[i+1][j]) >> 2;
        }
      }

      //MPI_Send(&local_new[0][0], number_of_recv_lines*DIM, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);
      for(int j = 0; j < number_of_recv_lines; j++)
      {
        //MPI_Send(&local_new[j][0], DIM, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);
        for(int k = 0; k < PACKETS_PER_LINE; k++)
        {
          MPI_Send(&local_new[j][k*PACKETLENGTH], PACKETLENGTH, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);
        }
      }

    }

    //printf("Calculation finished.\n");
  }

  MPI_Finalize();
  return 0;
}


