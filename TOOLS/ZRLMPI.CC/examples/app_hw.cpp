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
#include "ZRLMPI.hpp"
#include "app_hw.hpp"


int app_main(
    // ----- MPI_Interface -----
    stream<MPI_Interface> *soMPIif,
    stream<Axis<8> > *soMPI_data,
    stream<Axis<8> > *siMPI_data
    )
{
  int        rank, size;
  MPI_Status status;

  MPI_Init();

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );


  //int worker_nodes = size - 1; //root is not planning to do smth;

  printf("Here is rank %d, size is %d. \n",rank, size);


    int local_grid[WORKER_LINES + 1][DIM];
    int local_new[WORKER_LINES  + 1][DIM];

    int number_of_recv_lines = WORKER_LINES + 2;
    //if(rank == size -1 || rank == 1)
    //{
    //  number_of_recv_lines--;
    //}
    for(int l = 0; l < ITERATIONS; l++)
    {
      MPI_Recv(soMPIif, siMPI_data, &local_grid[0][0], number_of_recv_lines*DIM, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status);

      //#ifdef DEBUG
      //    print_array((const int*) local_grid, DIM, WORKER_LINES);
      //#endif

      //treat all borders equal, the additional lines in the middle are cut out from the merge at the server
      for(int i = 1; i < WORKER_LINES + 1; i++)
      {
        for(int j = 1; j<DIM-1; j++)
        {
          local_new[i][j] = (local_grid[i][j-1] + local_grid[i][j+1] + local_grid[i-1][j] + local_grid[i+1][j]) / 4.0;
        }
      }

      MPI_Send(soMPIif, soMPI_data, &local_new[0][0], number_of_recv_lines*DIM, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);

    }

    //printf("Calculation finished.\n");

  MPI_Finalize();
  return 0;
}


