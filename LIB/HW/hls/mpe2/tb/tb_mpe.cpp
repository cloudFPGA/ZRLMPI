
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

ap_uint<32> own_rank = 0;
//ap_uint<32> po_MMIO= 0;
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
     // &po_MMIO,
      MPIif_in,
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
  own_rank = 1;

  //MPI_send()
  strcpy(current_phase, "rank 1 MPI_send");

  MPI_Interface info = MPI_Interface();
  info.mpi_call = MPI_SEND_INT;
  info.count = 5;
  info.rank = 2;

  MPIif_in.write(info);

  char* msg = "HELLO WORLD! 1234\0\0\0\0";
  //17 payload bytes -> 5 words

  for(int i = 0; i< (17+7)/8; i++)
  {
    tmp64_2.tdata = 0x0;
    for(int k = 0; k<8; k++)
    {
      tmp64_2.tdata |= ((ap_uint<64>) msg[i*8+k]) << (3-k)*8;
    }
    if(i == (17+7/8)-1)
    {
      tmp64_2.tlast = 1;
    } else {
      tmp64_2.tlast = 0;
    }
    printf("TB: write MPI data: %#08x\n", (int) tmp64_2.tdata);
    MPI_data_in.write(tmp64_2);

    stepDut();
  }

  ap_uint<8> bytes[MPIF_HEADER_LENGTH];
  MPI_Header header = MPI_Header(); 

  stepDut();
  stepDut();
  stepDut();
  stepDut();
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
    if ( i == MPIF_HEADER_LENGTH - 1)
    {
      tmp.tlast = 1;
    }
    //tmp64Stream.write(tmp);
    siTcp_data.write(tmp);
    printf("Write Tcp word: %#016llx\n", (unsigned long long) tmp.tdata);
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
    if ( i == MPIF_HEADER_LENGTH - 1)
    {
      tmp.tlast = 1;
    }
    siTcp_data.write(tmp);
    printf("Write Tcp word: %#016llx\n", (unsigned long long) tmp.tdata);
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
  while(!soTcp_data.empty())
  {
    stepDut();
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
      if ( i == MPIF_HEADER_LENGTH - 1)
      {
        tmp.tlast = 1;
      }
      siTcp_data.write(tmp);
      printf("Write Tcp word: %#016llx\n", (unsigned long long) tmp.tdata);
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

  for(int i = 0; i< (17+3)/4; i++)
  {
    //tmp8 = MPI_data_out.read();
    if(!MPI_data_out.read_nb(tmp64_2))
    {
      break;
    }
    
    printf("MPI read data: %#08x, i: %d, tlast %d\n", (int) tmp64_2.tdata, i, (int) tmp64_2.tlast);

    stepDut();

    // tlast => i=11 
    //assert( !(tmp8.tlast == 1) || i==11);
    if(i == ((17+3)/4)-1)
    {
      assert(tmp64_2.tlast == 1);
    }
    //assert(((int) tmp8.tdata) == msg[i]);
    for(int k = 0; k<4; k++)
    {
      uint8_t cur_byte = (uint8_t) (tmp64_2.tdata >> (3-k)*8);
      //uint8_t cur_byte = (uint8_t) (tmp64_2.tdata >> k*8);
      //printf("cur byte: %02x\n", cur_byte);
      //printf("should be: %02x\n", (uint8_t) msg[i*4+k]);
      assert(cur_byte == msg[i*4+k]);
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
  


  printf("DONE\n");
  return succeded? 0 : -1;
  //return 0;
}