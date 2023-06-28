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

#ifndef _MPE_H_
#define _MPE_H_

#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

#include "../../../../../../cFDK/SRA/LIB/hls/axi_utils.hpp"
#include "../../../../../../cFDK/SRA/LIB/hls/network.hpp"
#include "../../../../../../cFDK/SRA/LIB/hls/cfdk.hpp"

#include "zrlmpi_common.hpp"

using namespace hls;


//currently not used --> new with MCC connection?
//#define NUMBER_STATUS_WORDS 16
//
//#define MPE_STATUS_UNUSED_1 1
//#define MPE_STATUS_UNUSED_2 2
//#define MPE_STATUS_UNUSED_3 3
//
//#define MPE_STATUS_WRITE_ERROR_CNT 4
//#define MPE_STATUS_READ_ERROR_CNT 5
//#define MPE_STATUS_SEND_STATE 6
//#define MPE_STATUS_RECEIVE_STATE 7
//#define MPE_STATUS_LAST_WRITE_ERROR 8
//#define MPE_STATUS_LAST_READ_ERROR 9
//#define MPE_STATUS_ERROR_HANDSHAKE_CNT 10
//#define MPE_STATUS_ACK_HANKSHAKE_CNT 11
//#define MPE_STATUS_GLOBAL_STATE 12
//#define MPE_STATUS_READOUT 13
////and 14 and 15
//
////Error types
//#define MPE_TX_INVALID_DST_RANK 0xd
//#define MPE_RX_INCOMPLETE_HEADER 0x1
//#define MPE_RX_INVALID_HEADER 0x2
//#define MPE_RX_IP_MISSMATCH 0x3
//#define MPE_RX_WRONG_DST_RANK 0x4
//#define MPE_RX_WRONG_DATA_TYPE 0x5
//#define MPE_RX_TIMEOUT 0x6


enum mpeState {MPE_RESET = 0, IDLE, START_SEND, START_SEND_1, SEND_REQ,
  //               0           1     2               3           4
  WAIT4CLEAR_CACHE_LOOKUP, WAIT4CLEAR_CACHE, WAIT4CLEAR, WAIT4CLEAR_1,
  //   5                       6               7             8
  SEND_DATA_START, SEND_DATA_START_1, SEND_DATA_RD, SEND_DATA_WRD,
  // 9                    10                11         12
  WAIT4ACK_CACHE_LOOKUP, WAIT4ACK_CACHE, WAIT4ACK_CACHE_2, WAIT4ACK, WAIT4ACK_1,
  //   13                  14              15               16           17
  START_RECV, WAIT4REQ_CACHE_LOOKUP, WAIT4REQ_CACHE, WAIT4REQ, WAIT4REQ_1, ASSEMBLE_CLEAR,
  //   18              19                    20         21         22          23
  ASSEMBLE_CLEAR_1, SEND_CLEAR, RECV_DATA_START, RECV_DATA_START_1,
  //    24               25         26                27
  RECV_DATA_RD, RECV_DATA_WRD, RECV_DATA_DONE, SEND_ACK_0, SEND_ACK,
  //   28           29              30            31          32
  MPE_DRAIN_DATA_STREAM};
  //   33


enum mpiType {MPI_INT = 0, MPI_FLOAT};

enum deqState {DEQ_IDLE = 0, DEQ_START, DEQ_WRITE, DEQ_WRITE_2, DEQ_DONE};


//#define HEADER_CACHE_LENGTH 64 //similar to max cluster size
#define HEADER_CACHE_LENGTH (ZRLMPI_MAX_CLUSTER_SIZE)
#define INVALID_CACHE_LINE_NUMBER 0xFEF //must be bigger than 64
#define BPL 16  //bytes per line
#define WPL 4   //words per line



void mpe_main(
    // ----- NAL Interface -----
    stream<NetworkWord>            &siTcp_data,
    stream<NetworkMetaStream>      &siTcp_meta,
    stream<NetworkWord>            &soTcp_data,
    stream<NetworkMetaStream>      &soTcp_meta,
    ap_uint<32>                   *po_rx_ports,

    ap_uint<32> *own_rank,
    // ----- for debugging  ------
    ap_uint<64> *MMIO_out,

    // ----- MPI_Interface -----
    stream<MPI_Interface> &siMPIif,
    stream<MPI_Feedback> &soMPIFeB,
    stream<Axis<64> > &siMPI_data,
    stream<Axis<64> > &soMPI_data
    );


#endif

