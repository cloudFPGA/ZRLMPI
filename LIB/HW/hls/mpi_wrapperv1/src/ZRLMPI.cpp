#include <stdio.h>
#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

#include "app_hw.hpp"
#include "ZRLMPI.hpp"

using namespace hls;


ap_uint<32> role_rank;
ap_uint<32> cluster_size;

ap_uint<1> my_app_done = 0;
ap_uint<1> app_init = 0;
ap_uint<1> hw_app_init = 0;

ap_uint<4> sendCnt = 0;
ap_uint<4> recvCnt = 0;

  
uint8_t bytes[ZRLMPI_MAX_DETECTED_BUFFER_SIZE]; //TODO!
//#pragma HLS RESOURCE variable=bytes core=RAM_2P_BRAM

void setMMIO_out(ap_uint<16> *MMIO_out)
{
#pragma HLS inline
  ap_uint<16> Display0 = 0;

  Display0  = (ap_uint<16>) ((ap_uint<4>) role_rank);
  Display0 |= ((ap_uint<16>) sendCnt) << SEND_CNT_SHIFT;
  Display0 |= ((ap_uint<16>) recvCnt) << RECV_CNT_SHIFT;
  Display0 |= ((ap_uint<16>) my_app_done) << AP_DONE_SHIFT;
  Display0 |= ((ap_uint<16>) app_init) << AP_INIT_SHIFT;

  *MMIO_out = Display0;
}

void MPI_Init()
{
#pragma HLS inline

  // INIT already done in wrapper_main
  printf("[MPI_Init] clusterSize: %d, rank: %d\n", (int) cluster_size, (int) role_rank);
  hw_app_init = 1;

}

void MPI_Comm_rank(MPI_Comm communicator, int* rank)
{
#pragma HLS inline
  *rank = role_rank;
}

void MPI_Comm_size(MPI_Comm communicator, int* size)
{
#pragma HLS inline
  *size = cluster_size;
}

int send_internal(
  stream<MPI_Interface> *soMPIif,
  stream<Axis<8> > *soMPI_data,
    uint8_t* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
{
#pragma HLS inline

  MPI_Interface info = MPI_Interface();
  info.rank = destination;

  //TODO: handle tag
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

  info.count = typeWidth*count;

  //we are using blocking calls, since we are in a blocking synchronization method
  WrapperSendState sendState = SEND_WRITE_INFO;
  uint32_t send_i = 0;
  //uint32_t send_i_per_packet = 0;
  while(sendState != SEND_DONE)
  {
    Axis<8>  tmp8 = Axis<8>(data[send_i]);
    switch(sendState) {
      case SEND_WRITE_INFO:
        //if(!soMPIif->full())
        //{
        soMPIif->write(info);
        sendState = SEND_WRITE_DATA;
        //}
        break;

      case SEND_WRITE_DATA:
        //  if(!soMPI_data->full())
        //  {

        //if(send_i_per_packet >= ZRLMPI_MAX_MESSAGE_SIZE_BYTES || //--> better by UOE/MPE? TODO
        if(send_i == info.count - 1)
        {
          tmp8.tlast = 1;
        } else {
          tmp8.tlast = 0;
        }
        printf("write MPI data: %#02x\n", (int) tmp8.tdata);
        soMPI_data->write(tmp8);
        send_i++;
        //send_i_per_packet++;
        if(send_i == info.count - 1)
        {
          sendState = SEND_FINISH;
        }
        //  }
        break;

      case SEND_FINISH:
        sendState = SEND_DONE;
        sendCnt++;
        printf("MPI_send completed.\n");
        //setMMIO_out(MMIO_out);
        break;
      
      default:
      case SEND_DONE:
        //NOP
        break;
    }
  }
  return 1;


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
#pragma HLS inline
  
  for(int i=0; i< count*4; i+=4)
  {
    bytes[i + 0] = (data[i/4] >> 24) & 0xFF;
    bytes[i + 1] = (data[i/4] >> 16) & 0xFF;
    bytes[i + 2] = (data[i/4] >> 8) & 0xFF;
    bytes[i + 3] = data[i/4] & 0xFF;
  }
  send_internal(soMPIif, soMPI_data, bytes, count,datatype, destination, tag, communicator);
}


int recv_internal(
  stream<MPI_Interface> *soMPIif,
  stream<Axis<8> > *siMPI_data,
  uint8_t* data,
  int count,
  MPI_Datatype datatype,
  int source,
  int tag,
  MPI_Comm communicator,
  MPI_Status* status)
{
#pragma HLS inline

  MPI_Interface info = MPI_Interface();
  info.rank = source;

  //TODO: handle tag
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

  //we are using blocking calls, since we are in a blocking synchronization method
  info.count = typeWidth * count;
  WrapperRecvState recvState = RECV_WRITE_INFO;
  int recv_i = 0;
  bool recv_tlastOccured = false;

  while(recvState != RECV_DONE)
  {
    switch(recvState) {

      case RECV_WRITE_INFO:
        //if(!soMPIif->full())
        //{
          soMPIif->write(info);
          recvState = RECV_READ_DATA;
        //}
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
          if(status != MPI_STATUS_IGNORE)
          {
            *status = 2;
          }
        } else {
          if(status != MPI_STATUS_IGNORE)
          {
            *status = 1;
          }

          recvCnt++;
          recvState = RECV_DONE;
          printf("MPI_recv completed.\n");
          //setMMIO_out(MMIO_out);

        }
        break;

      default:
      case RECV_DONE:
        //NOP
        break;
    }
  }
  
  return 1;
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
#pragma HLS inline

  recv_internal(soMPIif, siMPI_data, bytes, count, datatype, source, tag, communicator, status);

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
{
#pragma HLS inline
  my_app_done = 1;
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
  //#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE ap_vld register port=role_rank_arg name=piSMC_to_ROLE_rank
#pragma HLS INTERFACE ap_vld register port=cluster_size_arg name=piSMC_to_ROLE_size
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE ap_fifo port=soMPIif    //depth=16
#pragma HLS DATA_PACK     variable=soMPIif
#pragma HLS INTERFACE ap_fifo port=soMPI_data //depth=2048
#pragma HLS DATA_PACK     variable=soMPI_data
#pragma HLS INTERFACE ap_fifo port=siMPI_data //depth=2048
#pragma HLS DATA_PACK     variable=siMPI_data

#pragma HLS reset variable=my_app_done
#pragma HLS reset variable=sendCnt
#pragma HLS reset variable=recvCnt
#pragma HLS reset variable=app_init
#pragma HLS reset variable=hw_app_init
#pragma HLS reset variable=cluster_size
#pragma HLS reset variable=role_rank


  //#pragma HLS loop_flatten off 

  //===========================================================
  // Wait for INIT
  // nees do be done here, due to shitty HLS

  if(app_init == 0)
  {
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
    return;
  }

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

