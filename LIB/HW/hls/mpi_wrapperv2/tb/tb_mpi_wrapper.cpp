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

#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "../src/ZRLMPI.hpp"
#include "../src/app_hw.hpp"


void print_array(const int *A, size_t width, size_t height)
{
  printf("\n");
  for(size_t i = 0; i < height; i++)
  {
    printf("%#3d\t", i);
    for(size_t j = 0; j < width; j++)
    {
      printf("%*d ", 4, A[i * width + j]);
    }
    printf("\n");
  }
  printf("\n");
}


int main() 
{


  printf("---------------\n\tThis simulation works only with *one* iteration with DIM %d!\n\t(so it expects one recv and one send from the app)\n---------------\n",DIM);

  bool succeded = true;

  // ----- system reset ---
  //ap_uint<1> sys_reset = 0;
  //EMIF Registers
  ap_uint<16> MMIO_in = 0;
  ap_uint<16> MMIO_out = 0;
  // ----- MPI_Interface -----
  //stream<MPI_Interface> siMPIif;
  stream<MPI_Interface> soMPIif;
  stream<MPI_Feedback> siMPIFeB;
  stream<Axis<64> > siMPI_data;
  stream<Axis<64> > soMPI_data;
  // ----- FROM SMC -----
  ap_uint<32> role_rank = 0;
  ap_uint<32> cluster_size = 0;

  cluster_size = 2;
  role_rank = 1;


  int grid[DIM][DIM];
  int grid2[DIM][DIM];
  //fill with zeros and init borders with 4
  for(int i = 0; i<DIM; i++)
  {
    for(int j = 0; j<DIM; j++)
    {

      if( i == 0 || i == DIM-1 || j == 0 || j == DIM-1)
      {
        grid[i][j] = 128;
      }
      else {
        grid[i][j] = 0;
      }
      grid2[i][j] = 0;
    }
  }

  print_array((const int*) &grid, DIM, DIM);


  MPI_Interface info = MPI_Interface();
  info.mpi_call = MPI_SEND_INT;
  //info.count = (DIM/cluster_size + 1)*4;
  info.count = (DIM/cluster_size)*DIM; //in Words!
  info.rank = 0;

  //siMPIif.write(info);

  //char* data = (char* ) grid;
  //Axis<8>  tmp8 = Axis<8>();
  uint32_t* data = (uint32_t* ) grid;

  for(int i = 0; i< info.count; i+=2)
  {
    Axis<64>  tmp64 = Axis<64>();
    tmp64.tdata = (ap_uint<64>) data[i];
    tmp64.tkeep = 0xFF;
    if(i< info.count-1)
    {
      tmp64.tdata |= ((ap_uint<64>) data[i+1]) << 32;
    }
    if(i >= info.count - 2)
    {
      tmp64.tlast = 1;
    } else {
      tmp64.tlast = 0;
    }
    //printf("write MPI .tdata: %#02x, .tkeep %d, .tlast %d\n", (int) tmp8.tdata, (int) tmp8.tkeep, (int) tmp8.tlast);
    printf("[TB][%#3d] write MPI .tdata: %#016llx, .tkeep %#02x, .tlast %d\n", i, (uint64_t) tmp64.tdata, (int) tmp64.tkeep, (int) tmp64.tlast);
    siMPI_data.write(tmp64);
  }

  //for this _simple_ simulation, write fedback in advance
  siMPIFeB.write(ZRLMPI_FEEDBACK_OK);
  siMPIFeB.write(ZRLMPI_FEEDBACK_OK);


  //mpi_wrapper(sys_reset, role_rank, cluster_size);
  mpi_wrapper(role_rank, cluster_size, &MMIO_out, &soMPIif, &siMPIFeB, &soMPI_data, &siMPI_data);
  //twice, due to MPI_Init
  mpi_wrapper(role_rank, cluster_size, &MMIO_out, &soMPIif, &siMPIFeB, &soMPI_data, &siMPI_data);

  //empty streams
  info = soMPIif.read();
  assert(info.mpi_call == MPI_RECV_INT);
  info = soMPIif.read();
  assert(info.mpi_call == MPI_SEND_INT);


  uint32_t* recv_data = (uint32_t*) grid2;

  for(int i = 0; i< info.count; i+=2)
  {//not while...because we want test if there are exactly i bytes left 
    Axis<64>  tmp64= Axis<64>();
    soMPI_data.read(tmp64);
    printf("[TB][%#3d] read MPI .tdata: %#0llx, .tkeep %#02x, .tlast %d\n", i, (uint64_t) tmp64.tdata, (int) tmp64.tkeep, (int) tmp64.tlast);
    recv_data[i] = (uint32_t) tmp64.tdata;
    if(i < info.count -1)
    {
      recv_data[i+1] = (uint32_t) (tmp64.tdata >> 32);
    }
    if(i >= info.count -1)
    {
      assert(tmp64.tlast == 1);
    }
  }

  printf("\n[TB]  [ -- due to no border handling, the first line has random data -- ]\n");
  print_array((const int*) &grid2, DIM, DIM);

  printf("MMIO_out: %#02x\n",(int) MMIO_out);

  assert(MMIO_out == 0x5111);

  printf("DONE\n");

  assert(soMPIif.empty());
  assert(soMPI_data.empty());
  assert(siMPI_data.empty());
  assert(siMPIFeB.empty());

  return succeded? 0 : -1;
  //return 0;
}

