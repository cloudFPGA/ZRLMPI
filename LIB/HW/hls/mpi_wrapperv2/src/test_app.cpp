
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ZRLMPI.hpp"
#include "app_hw.hpp"



int app_main(
    // ----- MPI_Interface -----
    stream<MPI_Interface> *soMPIif,
    stream<MPI_Feedback> *siMPIFeB,
    stream<Axis<64> > *soMPI_data,
    stream<Axis<64> > *siMPI_data
    )
{
  int rank;
  int size;
  uint8_t status;
  int iterations[1];
  int start_line;
  int end_line;
  int result_start_line;
  MPI_Init();
  MPI_Comm_rank(0, &rank);
  size = 2;
  int (*grid)[16];
  printf("Here is rank %d, size is %d. \n", rank, size);
  ;
  int local_dim = 8;
  int local_grid[10][16];
  int local_new[10][16];
  //printf("Waiting for iterations...\n");
  //MPI_Recv(soMPIif, siMPI_data, &iterations[0], 1, 0, 0, 0, 0, &status);
  iterations[0] = 1;
  printf("...we are doing %d iterations.\n", iterations[0]);
  start_line = 1;
  end_line = 8;
  result_start_line = 1;
  int absoulte_start_line = rank * local_dim;
  int absoulte_end_line = ((rank + 1) * local_dim) - 1;
  int border_startline = 1;
  int border_endline = 8;
  border_startline--;
  ;
  for (int l = 0; l < iterations[0]; l++)
  {
    MPI_Recv(soMPIif, siMPIFeB, siMPI_data, &local_grid[start_line][0], local_dim * 16, 0, 0, 0, 0, 0);
    //MPI_Recv(soMPIif, siMPI_data, &local_grid[border_startline][0], 16, 0, 0, 0, 0, &status);
    //MPI_Recv(soMPIif, siMPI_data, &local_grid[border_endline][0], 16, 0, 0, 0, 0, &status);
    for (int i = 1; i < 9; i++)
    {
      for (int j = 0; j < 16; j++)
      {
        if (((((i == 1) && (absoulte_start_line == 0)) || ((i == local_dim) && (absoulte_end_line == 15))) || (j == 0)) || (j == 15))
        {
          local_new[i][j] = local_grid[i][j];
        }
        else
        {
          local_new[i][j] = (((local_grid[i][j - 1] + local_grid[i][j + 1]) + local_grid[i - 1][j]) + local_grid[i + 1][j]) >> 2;
        }

      }

    }

    MPI_Send(soMPIif, siMPIFeB, soMPI_data, &local_new[result_start_line][0], local_dim * 16, 0, 0, 0, 0);
  }

  ;
  MPI_Finalize();
  return 0;
}

