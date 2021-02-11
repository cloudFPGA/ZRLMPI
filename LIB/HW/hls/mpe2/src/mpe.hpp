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

#define NUMBER_STATUS_WORDS 16

#define MPE_STATUS_UNUSED_1 1
#define MPE_STATUS_UNUSED_2 2
#define MPE_STATUS_UNUSED_3 3

#define MPE_STATUS_WRITE_ERROR_CNT 4
#define MPE_STATUS_READ_ERROR_CNT 5
#define MPE_STATUS_SEND_STATE 6
#define MPE_STATUS_RECEIVE_STATE 7
#define MPE_STATUS_LAST_WRITE_ERROR 8
#define MPE_STATUS_LAST_READ_ERROR 9
#define MPE_STATUS_ERROR_HANDSHAKE_CNT 10
#define MPE_STATUS_ACK_HANKSHAKE_CNT 11
#define MPE_STATUS_GLOBAL_STATE 12
#define MPE_STATUS_READOUT 13
//and 14 and 15 

//Error types
#define MPE_TX_INVALID_DST_RANK 0xd
#define MPE_RX_INCOMPLETE_HEADER 0x1
#define MPE_RX_INVALID_HEADER 0x2
#define MPE_RX_IP_MISSMATCH 0x3
#define MPE_RX_WRONG_DST_RANK 0x4
#define MPE_RX_WRONG_DATA_TYPE 0x5
#define MPE_RX_TIMEOUT 0x6


//FSM state types

//#define WRITE_IDLE 0
//#define WRITE_START 1
//#define WRITE_DATA 2
//#define WRITE_ERROR 3
//#define WRITE_STANDBY 4
//#define sendState uint8_t 
//
//
//#define READ_IDLE 0
//#define READ_DATA 2
//#define READ_ERROR 3
//#define READ_STANDBY 4
//#define receiveState uint8_t


//#define IDLE 0
//#define START_SEND 1
//#define SEND_REQ 2
//#define WAIT4CLEAR 3
//#define SEND_DATA_START 4
//#define SEND_DATA_RD 5
//#define SEND_DATA_WRD 6
//#define WAIT4ACK 7
////#define START_RECEIVE 8
//#define WAIT4REQ 9
//#define ASSEMBLE_CLEAR 10
//#define SEND_CLEAR 11
//#define RECV_DATA_START 12
//#define RECV_DATA_RD 13
//#define RECV_DATA_WRD 14
//#define RECV_DATA_DONE 15
//#define RECV_DATA_ERROR 16
//#define SEND_ACK 17
//#define mpeState uint8_t
enum mpeState {IDLE = 0, START_SEND, START_SEND_1, SEND_REQ,
	           WAIT4CLEAR_CACHE, WAIT4CLEAR, WAIT4CLEAR_1,
	           SEND_DATA_START, SEND_DATA_START_1, SEND_DATA_RD, SEND_DATA_WRD,
			   WAIT4ACK_CACHE, WAIT4ACK, WAIT4ACK_1,
	           START_RECV, WAIT4REQ_CACHE, WAIT4REQ, WAIT4REQ_1, ASSEMBLE_CLEAR,
			   ASSEMBLE_CLEAR_1, SEND_CLEAR, RECV_DATA_START, RECV_DATA_START_1,
			   RECV_DATA_RD, RECV_DATA_WRD, RECV_DATA_DONE, SEND_ACK_0, SEND_ACK};


//#define MPI_INT 0
//#define MPI_FLOAT 1
//#define mpiType uint8_t
enum mpiType {MPI_INT = 0, MPI_FLOAT};

//#define DEQ_IDLE 0
//#define DEQ_WRITE 1
//#define DEQ_DONE 3
//#define deqState uint8_t
enum deqState {DEQ_IDLE = 0, DEQ_WRITE, DEQ_DONE};

//ap_uint<32> littleEndianToInteger(ap_uint<8> *buffer, int lsb);
//void integerToLittleEndian(ap_uint<32> n, ap_uint<8> *bytes);

#define HEADER_CACHE_LENGTH 64 //similar to max cluster size
#define INVALID_CACHE_LINE_NUMBER 0xFEF //must be bigger than 64


//void convertAxisToNtsWidth(stream<Axis<8> > &small, NetworkWord &out);
//void convertAxisToNtsWidth(stream<Axis<32> > &small, NetworkWord &out);
//void convertAxisToMpiWidth(NetworkWord big, stream<Axis<8> > &out);
//void convertAxisToMpiWidth(NetworkWord big, stream<Axis<32> > &out);

//int bytesToHeader(ap_uint<8> bytes[MPIF_HEADER_LENGTH], MPI_Header &header);
//void headerToBytes(MPI_Header header, ap_uint<8> bytes[MPIF_HEADER_LENGTH]);

void mpe_main(
	    // ----- NAL Interface -----
	    stream<NetworkWord>            &siTcp_data,
	    stream<NetworkMetaStream>      &siTcp_meta,
	    stream<NetworkWord>            &soTcp_data,
	    stream<NetworkMetaStream>      &soTcp_meta,
	    ap_uint<32>                   *po_rx_ports,

	    ap_uint<32> *own_rank,
	    // ----- for debugging  ------
	    //ap_uint<32> *MMIO_out,

	    // ----- MPI_Interface -----
	    stream<MPI_Interface> &siMPIif,
	    stream<Axis<64> > &siMPI_data,
	    stream<Axis<64> > &soMPI_data
);


#endif

