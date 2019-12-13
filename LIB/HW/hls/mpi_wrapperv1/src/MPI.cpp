#include <stdio.h>
#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

#include "app_hw.hpp"
#include "MPI.hpp"

using namespace hls;


ap_uint<32> role_rank;
ap_uint<32> cluster_size;

ap_uint<1> my_app_done = 0;
ap_uint<1> app_init = 0;

ap_uint<4> sendCnt = 0;
ap_uint<4> recvCnt = 0;

WrapperRecvState recvState = 0;
int recv_i = 0;
bool recv_tlastOccured = false;
WrapperSendState sendState = 0;
int send_i = 0;

void setMMIO_out(ap_uint<16> *MMIO_out)
{
  ap_uint<16> Display0 = 0;

  Display0  = (ap_uint<16>) ((ap_uint<4>) role_rank);
  Display0 |= ((ap_uint<16>) sendCnt) << SEND_CNT_SHIFT;
  Display0 |= ((ap_uint<16>) recvCnt) << RECV_CNT_SHIFT;
  Display0 |= ((ap_uint<16>) my_app_done) << AP_DONE_SHIFT;
  Display0 |= ((ap_uint<16>) app_init) << AP_INIT_SHIFT;

  *MMIO_out = Display0;
}

//void MPI_Init(int* argc, char*** argv)
void MPI_Init()
//void MPI_Init(ap_uint<16> *MMIO_out)
{

  //TODO: send/wait for INIT packets? 
  // INIT already done in wrapper_main

  printf("clusterSize: %d, rank: %d\n", (int) cluster_size, (int) role_rank);

  app_init = 1;
  //setMMIO_out(MMIO_out);

}

void MPI_Comm_rank(MPI_Comm communicator, int* rank)
{
  *rank = role_rank;
  //setMMIO_out(MMIO_out);
}

void MPI_Comm_size(MPI_Comm communicator, int* size)
{
  *size = cluster_size;
  //setMMIO_out(MMIO_out);
}

int send_internal(
	stream<MPI_Interface> *soMPIif,
	stream<Axis<8> > *soMPI_data,
    uint8_t* data,
    int count,
    MPI_Datatype datatype,
    int destination)
{

  MPI_Interface info = MPI_Interface();
  info.rank = destination;

  //tag is not yet implemented

  int typeWidth = 1;

  switch(datatype)
  {
    case MPI_INTEGER:
      info.mpi_call = MPI_SEND_INT;
      typeWidth = 4;
      break;
    case MPI_FLOAT:
      info.mpi_call = MPI_SEND_FLOAT;
      typeWidth = 4;
      break;
    default:
      //not yet implemented 
      return 1;
  }

  info.count = typeWidth * count;

  switch(sendState) {
    case SEND_WRITE_INFO:
      if(!soMPIif->full())
      {
        soMPIif->write(info);
        send_i = 0;
        sendState = SEND_WRITE_DATA;
      }
      break;

    case SEND_WRITE_DATA:

      if(!soMPI_data->full())
      {
        Axis<8>  tmp8 = Axis<8>(data[send_i]);

        if(send_i == info.count - 1)
        {
          sendState = SEND_FINISH;
          tmp8.tlast = 1;
        } else {
          tmp8.tlast = 0;
        }
        printf("write MPI data: %#02x\n", (int) tmp8.tdata);

        soMPI_data->write(tmp8);
        send_i++;
      }
      break;

    case SEND_FINISH:

      sendCnt++;
      printf("MPI_send completed.\n");
      //setMMIO_out(MMIO_out);
      return 1;
      //break;
  }

  return -1;

}

void MPI_Send(
	// ----- MPI_Interface -----
	stream<MPI_Interface> *soMPIif,
	stream<Axis<8> > *soMPI_data,
	// ----- MPI Signature -----
    int* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
{
  //INT Version, so datatype should always be MPI_INTEGER

  uint8_t bytes[4*count];
#pragma HLS RESOURCE variable=bytes core=RAM_2P_BRAM

  for(int i=0; i<count; i++)
  {
    bytes[i*4 + 0] = (data[i] >> 24) & 0xFF;
    bytes[i*4 + 1] = (data[i] >> 16) & 0xFF;
    bytes[i*4 + 2] = (data[i] >> 8) & 0xFF;
    bytes[i*4 + 3] = data[i] & 0xFF;
  }

  //DUE TO SHITTY HLS...
  sendState = SEND_WRITE_INFO;
  send_i = 0;
  while(send_internal(soMPIif, soMPI_data, bytes,count,datatype,destination) != 1)
  {
    ap_wait_n(WAIT_CYCLES);
  }

}

int recv_internal(
	stream<MPI_Interface> *soMPIif,
	stream<Axis<8> > *siMPI_data,
    uint8_t* data,
    int count,
    MPI_Datatype datatype,
    int source,
    MPI_Status* status)
{

  MPI_Interface info = MPI_Interface();
  info.rank = source;

  //tag is not yet implemented

  int typeWidth = 1;

  switch(datatype)
  {
    case MPI_INTEGER:
      info.mpi_call = MPI_RECV_INT;
      typeWidth = 4;
      break;
    case MPI_FLOAT:
      info.mpi_call = MPI_RECV_FLOAT;
      typeWidth = 4;
      break;
    default:
      //not yet implemented 
      return 1;
  }

  info.count = typeWidth * count;

  switch(recvState) {

    case RECV_WRITE_INFO:
      if(!soMPIif->full())
      {
        soMPIif->write(info);
        recvState = RECV_READ_DATA;
        recv_i = 0;
      }
      break;
    case RECV_READ_DATA:

      if(!siMPI_data->empty())
      {
        Axis<8> tmp8 = siMPI_data->read();
        printf("read MPI data: %#02x\n", (int) tmp8.tdata);

        data[recv_i] = (uint8_t) tmp8.tdata;
        recv_i++;

        if( tmp8.tlast == 1)
        {
          printf("received TLAST at count %d!\n", recv_i);
          recv_tlastOccured = true;
          recvState = RECV_FINISH;
          //break;
        }
        if(recv_i == info.count)
        {
          recvState = RECV_FINISH;
        }
      }
      break;
    case RECV_FINISH:

      //TODO empty stream?
      if(!recv_tlastOccured && !siMPI_data->empty() )
      {
        printf("received stream longer than count!\n");
        Axis<8>  tmp8 = siMPI_data->read();
        *status = 2;
      } else {
        *status = 1;

        recvCnt++;
        printf("MPI_recv completed.\n");
        //setMMIO_out(MMIO_out);

        return 1;
      }
      break;
  }

  return -1;

}


void MPI_Recv(
	// ----- MPI_Interface -----
	stream<MPI_Interface> *soMPIif,
	stream<Axis<8> > *siMPI_data,
	// ----- MPI Signature -----
    int* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status)
{

  //INT Version, so datatype should always be MPI_INTEGER

  uint8_t bytes[4*count];
#pragma HLS RESOURCE variable=bytes core=RAM_2P_BRAM

  //DUE TO SHITTY HLS...
  recvState = RECV_WRITE_INFO;
  recv_i = 0;
  while( recv_internal(soMPIif, siMPI_data, bytes, count, datatype, source, status) != 1)
  {
    ap_wait_n(WAIT_CYCLES);
  }

  for(int i=0; i<count; i++)
  {
    data[i]  = 0;
    data[i]  = ((int) bytes[i*4 + 3]);
    data[i] |= ((int) bytes[i*4 + 2]) << 8;
    data[i] |= ((int) bytes[i*4 + 1]) << 16;
    data[i] |= ((int) bytes[i*4 + 0]) << 24;
  }


}

void MPI_Finalize()
//void MPI_Finalize(ap_uint<16> *MMIO_out)
{
  //TODO: send something like DONE packets?
  my_app_done = 1;
  app_init = 0;
  //setMMIO_out(MMIO_out);
}

/*void MPI_Barrier(MPI_Comm communicator)
  {
//not yet implemented
}*/


void mpi_wrapper(
    // ----- FROM SMC -----
    ap_uint<32> role_rank_arg,
    ap_uint<32> cluster_size_arg,
	// ----- TO SMC ------
	ap_uint<16> *MMIO_out,
	// ----- MPI_Interface -----
	//stream<MPI_Interface> *siMPIif,
	stream<MPI_Interface> *soMPIif,
	stream<Axis<8> > *soMPI_data,
	stream<Axis<8> > *siMPI_data
    )
{
//#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE ap_vld register port=sys_reset name=piSysReset
//#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE ap_vld register port=role_rank_arg name=piSMC_to_ROLE_rank
#pragma HLS INTERFACE ap_vld register port=cluster_size_arg name=piSMC_to_ROLE_size
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE axis register both port=soMPIif    //depth=16
#pragma HLS INTERFACE axis register both port=soMPI_data //depth=2048
//#pragma HLS INTERFACE axis register both port=siMPIif    //depth=16
#pragma HLS INTERFACE axis register both port=siMPI_data //depth=2048

#pragma HLS reset variable=my_app_done
#pragma HLS reset variable=sendCnt
#pragma HLS reset variable=recvCnt
#pragma HLS reset variable=app_init
#pragma HLS reset variable=cluster_size
#pragma HLS reset variable=role_rank

  //===========================================================
  // Wait for INIT
  // nees do be done here, due to shitty HLS

  if(cluster_size_arg == 0)
  {
    //not yet initialized
    printf("cluster size not yet set!\n");

    setMMIO_out(MMIO_out);
    return;

  }
  cluster_size = cluster_size_arg;
  role_rank = role_rank_arg;
  printf("clusterSize: %d, rank: %d\n", (int) cluster_size, (int) role_rank);

  app_init = 1;

  setMMIO_out(MMIO_out);

  //===========================================================
  // Start main program 

  if(my_app_done == 0)
  {
    //app_main(MMIO_out, soMPIif, soMPI_data, siMPI_data);
    app_main(soMPIif, soMPI_data, siMPI_data);
  }

 // at the end
  setMMIO_out(MMIO_out);

}
