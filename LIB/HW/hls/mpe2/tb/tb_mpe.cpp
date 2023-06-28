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

#include <stdio.h>
#include <stdlib.h>

#include "../src/zrlmpi_common.hpp"
#include "../src/mpe.hpp"

#include <stdint.h>

using namespace hls;

//test variablen as global data
bool succeded = true;
//ap_uint<32> MRT[MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
stream<NetworkWord> siTcp_data, storeSEND_REQ, storeData;
stream<NetworkMetaStream>  siTcp_meta;
stream<NetworkWord> soTcp_data;
stream<NetworkMetaStream> soTcp_meta;
stream<MPI_Feedback> soMPIFeB;

ap_uint<32> own_rank = 0;
ap_uint<64> po_MMIO = 0;
ap_uint<32> poMPE_rx_ports = 0;

stream<MPI_Interface> MPIif_in;
//stream<MPI_Interface> MPIif_out;
stream<Axis<64> > MPI_data_in;
stream<Axis<64> > MPI_data_out;//, tmp64Stream;

unsigned int         simCnt;
char current_phase[101];
char last_phase[101];

void stepDut() {
  current_phase[100] = '\0';
  last_phase[100] = '\0';
  if(strncmp(current_phase, last_phase,101) != 0)
  {
    printf("\t\t +++++ [Test phase %s] +++++ \n",current_phase);
    strncpy(last_phase,current_phase,101);
  }
  mpe_main(
      //MRT,
      siTcp_data, siTcp_meta,
      soTcp_data, soTcp_meta,
      &poMPE_rx_ports, &own_rank,
      &po_MMIO,
      MPIif_in, soMPIFeB,
      MPI_data_in, MPI_data_out
      );
  simCnt++;
  printf("[%4.4d] STEP DUT \n", simCnt);
  fflush(stdout);
  fflush(stderr);
}


int main(){

  current_phase[100] = '\0';
  last_phase[100] = '\0';

  //for(int i = 0; i < MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS; i++)
  //{
  //      MRT[i] = 0;
  //}

  //MRT[0] = 1; //own rank 
  ////routing table
  //MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0] = 168496129; //10.11.12.1
  //MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 1] = 168496141; //10.11.12.13
  //MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2] = 168496142; //10.11.12.14
  //stepDut();


  //printf("MRT 3: %d\n",(int) MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 3]);

  //check DEBUG copies
  //assert(MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0] == MRT[NUMBER_CONFIG_WORDS + 0]);
  //assert(MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 1] == MRT[NUMBER_CONFIG_WORDS + 1]);
  //assert(MRT[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2] == MRT[NUMBER_CONFIG_WORDS + 2]);

  //init?
  stepDut(); //1
  assert(poMPE_rx_ports == 1);

  NetworkWord tmp64 = NetworkWord();
  //Axis<8>  tmp8 = Axis<8>();
  Axis<64>  tmp64_2 = Axis<64>();
  ap_uint<64> tmp64_2_tdata = 0x0;
  own_rank = 1;

  //MPI_send()
  strcpy(current_phase, "rank 1 MPI_send");

  MPI_Interface info = MPI_Interface();
  info.mpi_call = MPI_SEND_INT;
  info.count = 5; //in WORDS!
  info.rank = 2;

  MPIif_in.write(info);

  char* msg = "HELLO WORLD! 1234\0\0\0\0\0\0\0\0\0\0\0";
  //17 payload bytes -> 5 words --> 3 lines

  for(int i = 0; i< (17+7)/8; i++)
  {
    tmp64_2_tdata = 0x0;
    for(int k = 0; k<8; k++)
    {
      tmp64_2_tdata |= ((ap_uint<64>) msg[i*8+k]) << (7-k)*8;
      //printf("tdata construction: %#0llx\n", (uint64_t) tmp64_2.tdata);
    }
    tmp64_2.setTData(tmp64_2_tdata);
    if(i >= ((17+7)/8)-1)
    {
      tmp64_2.setTLast(1);
    } else {
      tmp64_2.setTLast(0);
    }
    tmp64_2.setTKeep(0xFF);
    printf("TB: write MPI data: %#0llx (last %d)\n", (uint64_t) tmp64_2.getTData(), (int) tmp64_2.getTLast());
    MPI_data_in.write(tmp64_2);

    stepDut();
  }

  ap_uint<8> bytes[MPIF_HEADER_LENGTH];
  MPI_Header header = MPI_Header(); 


  stepDut();
  //SEND_REQUEST expected
  NetworkMeta out_meta = soTcp_meta.read().tdata;
  printf("Dst node id: %d\n", (unsigned int) out_meta.dst_rank);
  assert(out_meta.dst_rank == 2);


  //while(!soTcp.empty())
  for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
  {
    stepDut();
    tmp64 = soTcp_data.read();
    printf("MPE out: %#016llx\n", (unsigned long long) tmp64.tdata);
    //we test the cache
    //storeSEND_REQ.write(tmp64);
    for(int j = 0; j<8; j++)
    {
      bytes[i*8 + j] = (ap_uint<8>) ( tmp64.tdata >> j*8) ;
      //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp64.tdata >> j*8) ;
    }
  }

  int ret = bytesToHeader(bytes, header);
  assert(ret == 0);
  assert(header.dst_rank == 2 && header.src_rank == 1); 
  assert(header.type == SEND_REQUEST);

  //first, send wrong clear to send (to check header cache)
  header = MPI_Header();
  //printf("send CLEAR_TO_SEND\n");
  strcpy(current_phase, "send (early) SEND_REQUEST");

  header.type = SEND_REQUEST;
  header.src_rank = 1;
  header.dst_rank = 2;
  header.size = 0;
  header.call = MPI_SEND_INT;
  headerToBytes(header, bytes);
  //write header
  for(int i = 0; i < MPIF_HEADER_LENGTH/8; i++)
  {
    //Axis<32> tmp = Axis<32>(bytes[i]);
    NetworkWord tmp = NetworkWord();
    tmp.tdata = 0x0;
    tmp.tkeep = 0xFF;
    for(int j = 0; j<8; j++)
    {
      //tmp.tdata |= (ap_uint<32>) (bytes[i*4+j] << (3-j)*8 );
      tmp.tdata |= ((ap_uint<64>) bytes[i*8+j]) << j*8;
    }
    // printf("tdata32: %#08x\n",(uint32_t) tmp.tdata);
    tmp.tlast = 0;
    if ( i == (MPIF_HEADER_LENGTH/8) - 1)
    {
      tmp.tlast = 1;
    }
    //tmp64Stream.write(tmp);
    siTcp_data.write(tmp);
    printf("Write Tcp word: %#016llx (last %d)\n", (unsigned long long) tmp.tdata, (int) tmp.tlast);
  }
  //  for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
  //  {
  //      convertAxisToNtsWidth(tmp64_2Stream, tmp64);
  //      siTcp_data.write(tmp64);
  //      printf("Write Tcp word: %#016llx\n", (unsigned long long) tmp64.tdata);
  //  }

  //meta does not fit header, but it is enough to test the cache
  NetworkMeta meta_1 = NetworkMeta(1,2718,2,2718,0);
  siTcp_meta.write(NetworkMetaStream(meta_1));

  for(int i = 0; i < 40; i++)
  {
    stepDut();
  }


  //assemble clear to send 
  header = MPI_Header();
  //printf("send CLEAR_TO_SEND\n");
  strcpy(current_phase, "send CLEAR_TO_SEND");

  header.type = CLEAR_TO_SEND;
  header.src_rank = 2;
  header.dst_rank = 1;
  header.size = 0;
  header.call = MPI_RECV_INT;
  headerToBytes(header, bytes);
  //write header
  for(int i = 0; i < MPIF_HEADER_LENGTH/8; i++)
  {
    //Axis<8> tmp = Axis<8>(bytes[i]);
    NetworkWord tmp = NetworkWord();
    tmp.tdata = 0x0;
    tmp.tkeep = 0xFF;
    for(int j = 0; j<8; j++)
    {
      tmp.tdata |= ((ap_uint<64>) bytes[i*8+j]) << j*8;
    }
    tmp.tlast = 0;
    if ( i == (MPIF_HEADER_LENGTH/8) - 1)
    {
      tmp.tlast = 1;
    }
    siTcp_data.write(tmp);
    printf("Write Tcp word: %#016llx (last %d)\n", (unsigned long long) tmp.tdata, (int) tmp.tlast);
  }
  //  for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
  //  {
  //      convertAxisToNtsWidth(tmp64_2Stream, tmp64);
  //      siTcp_data.write(tmp64);
  //      printf("Write Tcp word: %#016llx\n", (unsigned long long) tmp64.tdata);
  //  }

  meta_1 = NetworkMeta(1,2718,2,2718,0);
  siTcp_meta.write(NetworkMetaStream(meta_1));

  for(int i = 0; i < 40; i++)
  {
    stepDut();
  }

  //Data
  out_meta = soTcp_meta.read().tdata;
  printf("Dst node id: %d\n", (unsigned int) out_meta.dst_rank);
  assert(out_meta.dst_rank == 2);

  ap_uint<1> last_tlast = 0;
  //while(!soTcp_data.empty())
  for(int i = 0; i < 10; i++)
  {
    stepDut();
    if(soTcp_data.empty())
    {
      continue;
    }
    tmp64 = soTcp_data.read();
    printf("MPE out: %#016llx\n", (unsigned long long) tmp64.tdata);
    last_tlast = tmp64.tlast;
    storeData.write(tmp64);
  }
  assert(last_tlast == 1);

  //assemble ACK
  strcpy(current_phase, "send ACK");
  header = MPI_Header();

  header.type = ACK;
  header.src_rank = 2;
  header.dst_rank = 1;
  header.size = 0;
  header.call = MPI_RECV_INT;
  headerToBytes(header, bytes);
  //write header
  for(int i = 0; i < MPIF_HEADER_LENGTH/8; i++)
  {
    //Axis<8> tmp = Axis<8>(bytes[i]);
    NetworkWord tmp = NetworkWord();
    tmp.tdata = 0x0;
    tmp.tkeep = 0xFF;
    for(int j = 0; j<8; j++)
    {
      tmp.tdata |= ((ap_uint<64>) bytes[i*8+j]) << j*8;
    }
    tmp.tlast = 0;
    if ( i == (MPIF_HEADER_LENGTH/8) - 1)
    {
      tmp.tlast = 1;
    }
    siTcp_data.write(tmp);
    printf("Write Tcp word: %#016llx (last %d)\n", (unsigned long long) tmp.tdata, (int) tmp.tlast);
  }
  //  for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
  //  {
  //      convertAxisToNtsWidth(tmp64_2Stream, tmp64);
  //      siTcp_data.write(tmp64);
  //      printf("Write Tcp word: %#016llx\n", (unsigned long long) tmp64.tdata);
  //  }


  siTcp_meta.write(NetworkMetaStream(meta_1));
  //siTcp_meta.write(NetworkMetaStream(out_meta));

  for(int i = 0; i < 20; i++)
  {
    stepDut();
  }

  MPI_Feedback fedb = soMPIFeB.read();
  assert(fedb == ZRLMPI_FEEDBACK_OK);

  //MPI_recv()
  //printf("Start MPI_recv....\n");
  strcpy(current_phase, "rank 2 MPI_recv");
  //change rank to receiver 
  own_rank = 2;
  for(int i = 0; i < 3; i++)
  {
    stepDut();
  }

  info = MPI_Interface();
  info.mpi_call = MPI_RECV_INT;
  info.count = 5;
  info.rank = 1;

  MPIif_in.write(info);
  for(int i = 0; i < 3; i++)
  {
    stepDut();
  }
  //now in WAIT4REQ 
  NetworkMeta meta_2 = NetworkMeta(2,2718,1,2718,0);
  //siTcp_meta.write(NetworkMetaStream(meta_2));

  //for(int i = 0; i < 20; i++)
  //{
  //  if(!storeSEND_REQ.empty())
  //  {
  //      siTcp_data.write(storeSEND_REQ.read());
  //  }
  //}
  //
  //strcpy(current_phase, "send MPI REQUEST");
  //sould be in the cache
  strcpy(current_phase, "MPI REQUEST (from cache)");
  for(int i = 0; i < 20; i++)
  {
    stepDut();
  }
  //receive CLEAR_TO_SEND
  out_meta = soTcp_meta.read().tdata;
  printf("Dst node id: %d\n", (unsigned int) out_meta.dst_rank);
  assert(out_meta.dst_rank == 1);

  //while(!soTcp.empty())
  for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
  {
    stepDut();
    tmp64 = soTcp_data.read();
    printf("MPE out: %#016llx\n", (unsigned long long) tmp64.tdata);
    //storeSEND_REQ.write(tmp64);
    for(int j = 0; j<8; j++)
    {
      bytes[i*8 + j] = (ap_uint<8>) ( tmp64.tdata >> j*8) ;
      //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp64.tdata >> j*8) ;
    }
  }

  ret = bytesToHeader(bytes, header); 
  assert(ret == 0);
  assert(header.dst_rank == 1 && header.src_rank == 2); 
  assert(header.type == CLEAR_TO_SEND);

  printf("received CLEAR_TO_SEND.\n");

  //now send data and read from MPI
  for(int i = 0; i < 10; i++)
  {
    if(!storeData.empty())
    {
      siTcp_data.write(storeData.read());
    }
  }
  strcpy(current_phase, "rank 2 receive data");

  siTcp_meta.write(NetworkMetaStream(meta_2));

  for(int i = 0; i < 15; i++)
  {
    if(!storeData.empty())
    {
      siTcp_data.write(storeData.read());
    }
    stepDut();
  }
  //empty data
  //MPI_Interface info_out = MPIif_out.read();
  //assert(info_out.mpi_call == info.mpi_call);
  //assert(info_out.count == info.count);
  //assert(info_out.rank == 1);

  for(int i = 0; i< (17+7)/8; i++)
  {
    stepDut();

    //tmp8 = MPI_data_out.read();
    if(!MPI_data_out.read_nb(tmp64_2))
    {
      break;
    }

    printf("MPI read data: %#0llx, i: %d, tlast %d\n", (unsigned long long) tmp64_2.getTData(), i, (int) tmp64_2.getTLast());

    // tlast => i=11 
    //assert( !(tmp8.tlast == 1) || i==11);
    if(i == ((17+7)/8)-1)
    {
      assert(tmp64_2.getTLast() == 1);
    }
    //assert(((int) tmp8.tdata) == msg[i]);
    for(int k = 0; k<8; k++)
    {
      uint8_t cur_byte = (uint8_t) (tmp64_2.getTData() >> (7-k)*8);
      //uint8_t cur_byte = (uint8_t) (tmp64_2.tdata >> k*8);
      printf("cur byte: %02x\n", cur_byte);
      printf("should be: %02x\n", (uint8_t) msg[i*8+k]);
      assert(cur_byte == msg[i*8+k]);
    }
  }

  //receive ACK 
  strcpy(current_phase, "rank 2 receive ACK");
  for(int i = 0; i < 7; i++)
  {
    stepDut();
  }
  out_meta = soTcp_meta.read().tdata;
  printf("Dst node id: %d\n", (unsigned int) out_meta.dst_rank);
  assert(out_meta.dst_rank == 1);

  //while(!soTcp.empty())
  for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
  {
    stepDut();
    tmp64 = soTcp_data.read();
    printf("MPE out: %#016llx\n", (unsigned long long) tmp64.tdata);
    //storeSEND_REQ.write(tmp64);
    for(int j = 0; j<8; j++)
    {
      bytes[i*8 + j] = (ap_uint<8>) ( tmp64.tdata >> j*8) ;
      //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp64.tdata >> j*8) ;
    }
  }

  ret = bytesToHeader(bytes, header);
  assert(ret == 0);
  assert(header.dst_rank == 1 && header.src_rank == 2); 
  assert(header.type == ACK);

  printf("received ACK...\n");

  fedb = soMPIFeB.read();
  assert(fedb == ZRLMPI_FEEDBACK_OK);

  if(!MPI_data_in.empty())
  {
    printf("MPI_data_in not empty, this is not expected!\n");
    succeded = false;
  }
  if(!MPI_data_out.empty())
  {
    printf("MPI_data_out not empty, this is not expected!!\n");
    succeded = false;
  }

  printf("DONE\n");
  return succeded? 0 : -1;
  //return 0;
}

