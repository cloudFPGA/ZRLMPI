#ifndef _MPE_H_
#define _MPE_H_

#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

#include "../../../../../cFDK/SRA/LIB/hls/axi_utils.hpp"
#include "../../../../../cFDK/SRA/LIB/hls/network.hpp"
#include "../../../../../cFDK/SRA/LIB/hls/cfdk.hpp"

#include "zrlmpi_common.hpp"

using namespace hls;

#define WRITE_IDLE 0
#define WRITE_START 1
#define WRITE_DATA 2
#define WRITE_ERROR 3
#define WRITE_STANDBY 4
#define sendState uint8_t 


#define READ_IDLE 0
#define READ_DATA 2
#define READ_ERROR 3
#define READ_STANDBY 4
#define receiveState uint8_t


#define IDLE 0
#define START_SEND 1
#define SEND_REQ 9
#define WAIT4CLEAR 2 
#define SEND_DATA 3
#define WAIT4ACK 4
//#define START_RECEIVE 5
#define WAIT4REQ 6
#define SEND_CLEAR 10
#define RECV_DATA 7
#define SEND_ACK 8
#define mpeState uint8_t

#define MPI_INT 0
#define MPI_FLOAT 1
#define mpiType uint8_t


ap_uint<32> littleEndianToInteger(ap_uint<8> *buffer, int lsb);
void integerToLittleEndian(ap_uint<32> n, ap_uint<8> *bytes);


void convertAxisToNtsWidth(stream<Axis<8> > &small, Axis<64> &out);
void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out);
//int bytesToHeader(ap_uint<8> bytes[MPIF_HEADER_LENGTH], MPI_Header &header);
//void headerToBytes(MPI_Header header, ap_uint<8> bytes[MPIF_HEADER_LENGTH]);

void mpe_main(
    // ----- link to FMC -----
    // TODO: removed for now, FIXME: add to Management Companion Core in Middleware
    //ap_uint<32> ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],

    // ----- NRC Interface -----
    stream<NetworkWord>            &siTcpi_data,
    stream<NetowrkMetaStream>      &siTcp_meta,
    stream<NetworkWord>            &soTcp_data,
    stream<NetworkMetaStream>      &soTcp_meta,
    
    ap_uint<32> *own_rank,

    // ----- Memory -----
    //ap_uint<8> *MEM, TODO: maybe later

    // ----- MPI_Interface -----
    stream<MPI_Interface> &siMPIif,
    //stream<MPI_Interface> &soMPIif,
    stream<Axis<8> > &siMPI_data,
    stream<Axis<8> > &soMPI_data
);


#endif
