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

ap_uint<1> memory_test_flag = 0;


//uint8_t bytes[ZRLMPI_MAX_DETECTED_BUFFER_SIZE];
//uint32_t words[(ZRLMPI_MAX_DETECTED_BUFFER_SIZE+3)/4]; //TODO!
//#pragma HLS RESOURCE variable=words core=RAM_2P_BRAM

#ifdef DRAM_USED
uint32_t words[INTERNAL_BRAM_BUFFER_SIZE_WORDS];
#else
uint32_t words[64];
#endif

#define MEMORY_TEST_PATTERN 0x0A0B0C0D0E0FC001
bool memory_pattern_write = false;
bool memory_pattern_read = false;


void setMMIO_out(ap_uint<16> *MMIO_out)
{
#pragma HLS inline
  ap_uint<16> Display0 = 0;

  Display0  = (ap_uint<16>) ((ap_uint<4>) role_rank);
  Display0 |= ((ap_uint<16>) sendCnt) << SEND_CNT_SHIFT;
  Display0 |= ((ap_uint<16>) recvCnt) << RECV_CNT_SHIFT;
  Display0 |= ((ap_uint<16>) my_app_done) << AP_DONE_SHIFT;
  Display0 |= ((ap_uint<16>) app_init) << AP_INIT_SHIFT;
  Display0 |= ((ap_uint<16>) memory_test_flag) << MEM_ERR_SHIFT;

  *MMIO_out = Display0;
}


void my_exit(int status)
{
#pragma HLS inline
  printf("[ZRLMPI@FPGA] app_hw called 'exit' with status %d. This will be ignored.\n",status);
}

void my_free(void *ignore)
{
#pragma HLS inline
  printf("[ZRLMPI@FPGA] app_hw called 'free'. This will be ignored.\n");
}

void my_perror(const char *s)
{
#pragma HLS inline
  printf("[ZRLMPI@FPGA] app_hw called 'perror' with message: %s. This will be ignored.\n",s);
}

void MPI_Init()
{
#pragma HLS inline

  // INIT already done in wrapper_main
  printf("[MPI_Init] clusterSize: %d, rank: %d\n", (int) cluster_size, (int) role_rank);
  hw_app_init = 1;

}

void MPI_Init(void *a, void *b)
{
  MPI_Init();
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


void MPI_Send_DRAM(
    stream<MPI_Interface> *soMPIif,
    stream<MPI_Feedback> *siMPIFeB,
    stream<Axis<64> > *soMPI_data,
    int* data,
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
      return;
  }

  uint32_t word_count = typeWidth*count;
  info.count = word_count;

  //we are using blocking calls, since we are in a blocking synchronization method
  WrapperSendState sendState = SEND_WRITE_INFO;
  uint32_t send_i = 0;
  Axis<64>  tmp64 = Axis<64>();
  //uint32_t send_i_per_packet = 0;
  MPI_Feedback fedb;
  uint32_t address_copied = INTERNAL_BRAM_BUFFER_SIZE_WORDS;
  uint32_t max_copy_length = INTERNAL_BRAM_BUFFER_SIZE_WORDS * sizeof(int);
  uint32_t start_address = 0;
  if(address_copied > word_count)
  {
    max_copy_length = word_count * sizeof(int);
    address_copied = word_count;
  }
  memcpy(words, &data[start_address], max_copy_length);

  while(sendState != SEND_DONE)
  {
//#pragma HLS pipeline II=1
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
        if(send_i >= address_copied)
        {
          start_address = address_copied;
          if((word_count - send_i) < INTERNAL_BRAM_BUFFER_SIZE_WORDS)
          {
            address_copied += (word_count - send_i);
            max_copy_length = (word_count - send_i) * sizeof(int);
          } else {
            address_copied += INTERNAL_BRAM_BUFFER_SIZE_WORDS;
            max_copy_length = INTERNAL_BRAM_BUFFER_SIZE_WORDS * sizeof(int);
          }
          memcpy(words, &data[start_address], max_copy_length);
        }
        tmp64.tdata = 0x0;
        tmp64.tdata = (ap_uint<64>) words[send_i - start_address];
        tmp64.tkeep = 0xFF;
        if(send_i < (word_count -1))
        {
          tmp64.tdata |= ((ap_uint<64>) words[send_i +1 - start_address]) << 32;
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
          address_copied = INTERNAL_BRAM_BUFFER_SIZE_WORDS;
          max_copy_length = INTERNAL_BRAM_BUFFER_SIZE_WORDS * sizeof(int);
          start_address = 0;
          if(address_copied > word_count)
          {
            max_copy_length = word_count * sizeof(int);
            address_copied = word_count;
          }
          memcpy(words, &data[start_address], max_copy_length);
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
//#pragma HLS pipeline II=1

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
  //if(count > 1)
  //{
  //  for(int i=0; i< count; i++)
  //  {
  //    //#pragma HLS unroll
  //    words[i] = data[i];
  //    printf("\t[%02d] %04d\n", i, data[i]);
  //  }
  //  printf("\n");
  //} else {
  //  words[0] = data[0];
  //  printf("\t[00] %04d\n", data[0]);
  //}

  if(count > 1)
  {
    send_internal(soMPIif, siMPIFeB, soMPI_data, (uint32_t*) data, count, datatype, destination, tag, communicator);
  } else {
    //uint32_t small_words[1];
    //small_words[0] = data[0];
    //send_internal(soMPIif, siMPIFeB, soMPI_data, small_words, count, datatype, destination, tag, communicator);
    words[0] = data[0];
    send_internal(soMPIif, siMPIFeB, soMPI_data, words, count, datatype, destination, tag, communicator);
  }
}


void MPI_Recv_DRAM(
    stream<MPI_Interface> *soMPIif,
    stream<MPI_Feedback> *siMPIFeB,
    stream<Axis<64> > *siMPI_data,
    int* data,
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
      return;
  }

  //we are using blocking calls, since we are in a blocking synchronization method
  info.count = typeWidth * count;
  uint32_t word_count = info.count;
  WrapperRecvState recvState = RECV_WRITE_INFO;
  uint32_t recv_i = 0;
  bool recv_tlastOccured = false;
  Axis<64>  tmp64 = Axis<64>();
  MPI_Feedback fedb;
  uint32_t max_address_recv = INTERNAL_BRAM_BUFFER_SIZE_WORDS;
  uint32_t start_address = 0;
  uint32_t max_copy_length = INTERNAL_BRAM_BUFFER_SIZE_WORDS * sizeof(int);

  while(recvState != RECV_DONE)
  {
    //#pragma HLS loop_flatten off
    //#pragma HLS unroll factor=1
//#pragma HLS pipeline II=1

    switch(recvState) {

      case RECV_WRITE_INFO:
        //if(!soMPIif->full())
        //{
        soMPIif->write(info);
        recvState = RECV_READ_DATA;
        //}
        break;
      case RECV_READ_DATA:
        if(recv_i >= max_address_recv) //is %2=0, so should work...
        {
          memcpy(&data[start_address], words, max_copy_length);
          start_address += INTERNAL_BRAM_BUFFER_SIZE_WORDS;
          max_address_recv += INTERNAL_BRAM_BUFFER_SIZE_WORDS;
        }
        tmp64 = siMPI_data->read();
        printf("read MPI data: %#016llx\n", (uint64_t) tmp64.tdata);
        words[recv_i - start_address] = (uint32_t) tmp64.tdata;
        //we assume tkeep = 0xFF, becuase we sent it...
        if(recv_i < (word_count-1))
        {
          words[recv_i + 1 - start_address] = (uint32_t) (tmp64.tdata >> 32);
          recv_i += 2;
        } else {
          recv_i += 1;
        }

        if( tmp64.tlast == 1)
        {
          printf("received TLAST at count %d!\n", recv_i);
          recv_tlastOccured = true;
          recvState = RECV_FINISH;
        }
        //check length not necessary, we can trust the tlast from MPE
        break;
      case RECV_FINISH:
        max_copy_length = (recv_i - start_address) * sizeof(int);
        if(max_copy_length > 0)
        {
          memcpy(&data[start_address], words, max_copy_length);
        }

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
          max_address_recv = INTERNAL_BRAM_BUFFER_SIZE_WORDS;
          start_address = 0;
          max_copy_length = INTERNAL_BRAM_BUFFER_SIZE_WORDS * sizeof(int);
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
//#pragma HLS pipeline II=1

    switch(recvState) 
    {
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

  if(count > 1)
  {
    recv_internal(soMPIif, siMPIFeB, siMPI_data, (uint32_t*) data, count, datatype, source, tag, communicator, status);
  } else {
    //uint32_t small_words[1];
    //recv_internal(soMPIif, siMPIFeB, siMPI_data, small_words, count, datatype, source, tag, communicator, status);
    //data[0] = small_words[0];
    recv_internal(soMPIif, siMPIFeB, siMPI_data, words, count, datatype, source, tag, communicator, status);
    data[0] = words[0];
  }

  //for(int i=0; i<count; i++)
  //{
  //  data[i]  = 0;
  //  data[i]  = ((int) words[i*4 + 3]);
  //  data[i] |= ((int) words[i*4 + 2]) << 8;
  //  data[i] |= ((int) words[i*4 + 1]) << 16;
  //  data[i] |= ((int) words[i*4 + 0]) << 24;
  //}

  printf("[MPI_Recv] output data:\n");
  //if(count > 1) {
  //  for(int i=0; i<count; i++)
  //  {
  //    //#pragma HLS unroll
  //    data[i]  = (int) words[i];
  //    printf("\t[%02d] %04d\n", i, data[i]);
  //    //TODO: if count-1 (last word), handle non complete words (here not necessary)
  //  }
  //  printf("\n");
  //} else {
  //  data[0] = words[0];
  //  printf("\t[00] %04d\n", data[0]);
  //}

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
    stream<Axis<64> > *siMPI_data,
    // ----- DRAM -----
    int32_t boFdram[BOFDRAM_LINE_RESERVATION]
    /* buffer declarations that end up in memory are insereted below
     * if they exist */
    //ZRLMPI_BUFFER_DECLS
    // (this line will be replaced)
    )
{
  //#pragma HLS INTERFACE ap_ctrl_none port=return
  //#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE ap_vld register port=role_rank_arg name=piFMC_to_ROLE_rank
#pragma HLS INTERFACE ap_vld register port=cluster_size_arg name=piFMC_to_ROLE_size
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE ap_fifo port=soMPIif
#pragma HLS DATA_PACK     variable=soMPIif
#pragma HLS INTERFACE ap_fifo port=siMPIFeB
  //#pragma HLS DATA_PACK     variable=siMPIFeB
#pragma HLS INTERFACE ap_fifo port=soMPI_data
#pragma HLS DATA_PACK     variable=soMPI_data
#pragma HLS INTERFACE ap_fifo port=siMPI_data
#pragma HLS DATA_PACK     variable=siMPI_data

//#pragma HLS INTERFACE m_axi port=boFdram bundle=boAPP_DRAM offset=direct latency=52
#pragma HLS INTERFACE m_axi port=boFdram bundle=boAPP_DRAM offset=direct latency=52 num_read_outstanding=16 num_write_outstanding=16 max_read_burst_length=256 max_write_burst_length=256
//#pragma HLS array_map variable=words instance=boFdram horizontal //TODO

#pragma HLS reset variable=my_app_done
#pragma HLS reset variable=sendCnt
#pragma HLS reset variable=recvCnt
#pragma HLS reset variable=app_init
#pragma HLS reset variable=hw_app_init
#pragma HLS reset variable=cluster_size
#pragma HLS reset variable=role_rank

#pragma HLS reset variable=memory_pattern_write
#pragma HLS reset variable=memory_pattern_read
#pragma HLS reset variable=memory_test_flag

//#ifdef DRAM_USED
//  #pragma HLS ARRAY_PARTITION variable=words complete dim=1
//#endif


  //===========================================================
  // Wait for INIT
  // nees do be done here, due to shitty HLS

  if(app_init == 0 || !memory_pattern_read || !memory_pattern_write)
  {
    //use & test memory (also to avoid open connections if app is not using them)
    if(!memory_pattern_write)
    {
      boFdram[1] = (int32_t) MEMORY_TEST_PATTERN;
      memory_pattern_write = true;
      memory_test_flag = 0;
    } else if(!memory_pattern_read)
    {
      ap_uint<32> tmp = boFdram[1];
      if(tmp == (int32_t) MEMORY_TEST_PATTERN)
      {
        memory_test_flag = 1;
      }
      //stay 0 as "error"
      memory_pattern_read = true;
    }

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
    /* the call of app_main with correct buffers will be inserted by the Transpiler below */
    /* ZRLMPI_APP_MAIN_CALL -- ADD app_main_call here */
    // (this line will be replaced)
  }

  // at the end
  setMMIO_out(MMIO_out);

}

