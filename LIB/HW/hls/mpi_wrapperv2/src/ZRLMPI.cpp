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


//uint8_t bytes[ZRLMPI_MAX_DETECTED_BUFFER_SIZE];
uint32_t words[(ZRLMPI_MAX_DETECTED_BUFFER_SIZE+3)/4]; //TODO!
//#pragma HLS RESOURCE variable=words core=RAM_2P_BRAM

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


void my_memcpy(int * dst, int* src, int length)
{
  for(int i = 0; i < length/sizeof(int); i++)
  {
    dst[i] = src[i];
  }
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
    stream<MPI_Feedback> *siMPIFeB,
    stream<Axis<64> > *soMPI_data,
    uint32_t* data,
    int count,
    //uint8_t tkeep_last_word,
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
      //typeWidth = 4;
      typeWidth = 1;
      break;
    case MPI_FLOAT:
      info.mpi_call = MPI_SEND_FLOAT;
      //typeWidth = 4;
      typeWidth = 1;
      break;
    default:
      //not yet implemented 
      return 1;
  }

  uint32_t word_count = typeWidth*count;
  info.count = word_count;

  //we are using blocking calls, since we are in a blocking synchronization method
  WrapperSendState sendState = SEND_WRITE_INFO;
  uint32_t send_i = 0;
  Axis<64>  tmp64 = Axis<64>();
  //uint32_t send_i_per_packet = 0;
  MPI_Feedback fedb;
  
  while(sendState != SEND_DONE)
  {
    //#pragma HLS loop_flatten off
    //#pragma HLS unroll factor=1

    switch(sendState) {
      case SEND_WRITE_INFO:
        //if(!soMPIif->full())
        //{
        soMPIif->write(info);
        sendState = SEND_WRITE_DATA;
        //}
        break;

      case SEND_WRITE_DATA:
        tmp64.tdata = 0x0;
        tmp64.tdata = (ap_uint<64>) data[send_i];
        tmp64.tkeep = 0xFF;
        if(send_i < (word_count -1))
        {
          tmp64.tdata |= ((ap_uint<64>) data[send_i+1]) << 32;
        }
        //else, it stays 0
        send_i += 2;
        if(send_i >= word_count)
        {
          tmp64.tlast = 1;
          //keep is alway a full line
          sendState = SEND_FINISH;
        } else {
          tmp64.tlast = 0;
        }
        printf("write MPI data: %#016llx\n", (uint64_t) tmp64.tdata);
        //blocking!
        soMPI_data->write(tmp64);
        break;

      case SEND_FINISH:
        fedb = siMPIFeB->read();
        if(fedb == ZRLMPI_FEEDBACK_OK)
        {
          sendState = SEND_DONE;
          sendCnt++;
          printf("MPI_send completed.\n");
        } else {
          //timeout occured
          //go directly to write data is ok, since we've written TLAST before
          sendState = SEND_WRITE_DATA;
          send_i = 0;
        }

        //setMMIO_out(MMIO_out);
        break;

      default:
      case SEND_DONE:
        //NOP
        //ap_wait();
        break;
    }
  }
  return 1;


}


void MPI_Send(
    // ----- MPI_Interface -----
    stream<MPI_Interface> *soMPIif,
    stream<MPI_Feedback> *siMPIFeB,
    stream<Axis<64> > *soMPI_data,
    // ----- MPI Signature -----
    int* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
{
#pragma HLS inline

  //for(int i=0; i< count*4; i+=4)
  //{
  //  words[i + 0] = (data[i/4] >> 24) & 0xFF;
  //  words[i + 1] = (data[i/4] >> 16) & 0xFF;
  //  words[i + 2] = (data[i/4] >> 8) & 0xFF;
  //  words[i + 3] = data[i/4] & 0xFF;
  //}
  printf("[MPI_Send] input data:\n");
  for(int i=0; i< count; i++)
  {
    //#pragma HLS unroll
    words[i] = data[i];
    printf("\t[%02d] %04d\n", i, data[i]);
  }
  printf("\n");

  send_internal(soMPIif, siMPIFeB, soMPI_data, words, count, datatype, destination, tag, communicator);
}


int recv_internal(
    stream<MPI_Interface> *soMPIif,
    stream<MPI_Feedback> *siMPIFeB,
    stream<Axis<64> > *siMPI_data,
    uint32_t* data,
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
      //typeWidth = 4;
      typeWidth = 1;
      break;
    case MPI_FLOAT:
      info.mpi_call = MPI_RECV_FLOAT;
      //typeWidth = 4;
      typeWidth = 1;
      break;
    default:
      //not yet implemented 
      return 1;
  }

  //we are using blocking calls, since we are in a blocking synchronization method
  info.count = typeWidth * count;
  uint32_t word_count = info.count;
  WrapperRecvState recvState = RECV_WRITE_INFO;
  int recv_i = 0;
  bool recv_tlastOccured = false;
  Axis<64>  tmp64 = Axis<64>();
  MPI_Feedback fedb;

  while(recvState != RECV_DONE)
  {
    //#pragma HLS loop_flatten off
    //#pragma HLS unroll factor=1

    switch(recvState) {

      case RECV_WRITE_INFO:
        //if(!soMPIif->full())
        //{
        soMPIif->write(info);
        recvState = RECV_READ_DATA;
        //}
        break;
      case RECV_READ_DATA:

        tmp64 = siMPI_data->read();
        printf("read MPI data: %#016llx\n", (uint64_t) tmp64.tdata);

        data[recv_i] = (uint32_t) tmp64.tdata;
        //we assume tkeep = 0xFF, becuase we sent it...
        if(recv_i < (word_count-1))
        {
          data[recv_i+1] = (uint32_t) (tmp64.tdata >> 32);
        }
        recv_i +=2 ;

        if( tmp64.tlast == 1)
        {
          printf("received TLAST at count %d!\n", recv_i);
          recv_tlastOccured = true;
          recvState = RECV_FINISH;
        }
        //check length not necessary, we can trust the tlast from MPE
        break;
      case RECV_FINISH:

        fedb = siMPIFeB->read();
        if(fedb == ZRLMPI_FEEDBACK_OK)
        {
          if(status != MPI_STATUS_IGNORE)
          {
            *status = 1;
          }

          recvCnt++;
          recvState = RECV_DONE;
          printf("MPI_recv completed.\n");
        } else {
          //TIMEOUT occured...
          recvState = RECV_READ_DATA;
          recv_tlastOccured = false;
          recv_i = 0;
        }

        //setMMIO_out(MMIO_out);
        break;

      default:
      case RECV_DONE:
        //NOP
        //ap_wait();
        break;
    }
  }

  return 1;
}


void MPI_Recv(
    // ----- MPI_Interface -----
    stream<MPI_Interface> *soMPIif,
    stream<MPI_Feedback> *siMPIFeB,
    stream<Axis<64> > *siMPI_data,
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

  recv_internal(soMPIif, siMPIFeB, siMPI_data, words, count, datatype, source, tag, communicator, status);

  //for(int i=0; i<count; i++)
  //{
  //  data[i]  = 0;
  //  data[i]  = ((int) words[i*4 + 3]);
  //  data[i] |= ((int) words[i*4 + 2]) << 8;
  //  data[i] |= ((int) words[i*4 + 1]) << 16;
  //  data[i] |= ((int) words[i*4 + 0]) << 24;
  //}

  printf("[MPI_Recv] output data:\n");
  for(int i=0; i<count; i++)
  {
    //#pragma HLS unroll
    data[i]  = (int) words[i];
    printf("\t[%02d] %04d\n", i, data[i]);
    //TODO: if count-1 (last word), handle non complete words (here not necessary)
  }
  printf("\n");

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
    stream<MPI_Interface> *soMPIif,
    stream<MPI_Feedback> *siMPIFeB,
    stream<Axis<64> > *soMPI_data,
    stream<Axis<64> > *siMPI_data
    )
{
  //#pragma HLS INTERFACE ap_ctrl_none port=return
  //#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE ap_vld register port=role_rank_arg name=piSMC_to_ROLE_rank
#pragma HLS INTERFACE ap_vld register port=cluster_size_arg name=piSMC_to_ROLE_size
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE ap_fifo port=soMPIif
#pragma HLS DATA_PACK     variable=soMPIif
#pragma HLS INTERFACE ap_fifo port=siMPIFeB
//#pragma HLS DATA_PACK     variable=siMPIFeB
#pragma HLS INTERFACE ap_fifo port=soMPI_data
#pragma HLS DATA_PACK     variable=soMPI_data
#pragma HLS INTERFACE ap_fifo port=siMPI_data
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
    app_main(soMPIif, siMPIFeB, soMPI_data, siMPI_data);
  }

  // at the end
  setMMIO_out(MMIO_out);

}

