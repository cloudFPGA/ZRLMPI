#ifndef _ZRLMPI_COMMON_H_
#define _ZRLMPI_COMMON_H_

#include <stdint.h>

#include "zrlmpi_int.hpp"

#define MPI_SEND_INT 0
#define MPI_RECV_INT 1
#define MPI_SEND_FLOAT 2
#define MPI_RECV_FLOAT 3
#define MPI_BARRIER 4
#define mpiCall uint8_t

#define MPI_STATUS_IGNORE 0

#define SEND_REQUEST 1
#define CLEAR_TO_SEND 2
#define DATA 3
#define ACK 4
#define ERROR 5
#define packetType uint8_t

#define ZRLMPI_DEFAULT_PORT 2718

//UDP Header + MPIF Header = 74
//main constraint is ZYC2 VXLAN setup
#define ZRLMPI_MAX_MESSAGE_SIZE_BYTES 1416  //Bytes inclusive header!
#define ZRLMPI_MAX_MESSAGE_SIZE_WORDS 354   //int or float inclusive header!
#define ZRLMPI_MAX_MESSAGE_SIZE_LINES 177   //line of 8 byte (inclusive header)

/*
 * MPI-F Interface
 */
 struct MPI_Interface {
   UINT8     mpi_call;
   UINT32    count;
   UINT32    rank;
   MPI_Interface() {}
 };


/*
 * MPI-F Header 
 */
 struct MPI_Header {
  UINT32 dst_rank;
  UINT32 src_rank;
  UINT32 size; 
  mpiCall call;
  packetType type;
  MPI_Header() {}
 };
#define MPIF_HEADER_LENGTH 32 //Bytes


//UINT32 littleEndianToInteger(UINT8 *buffer, int lsb);
UINT32 bigEndianToInteger(UINT8 *buffer, int lsb);
void integerToBigEndian(UINT32 n, UINT8 *bytes);
int bytesToHeader(UINT8 bytes[MPIF_HEADER_LENGTH], MPI_Header &header);
void headerToBytes(MPI_Header header, UINT8 bytes[MPIF_HEADER_LENGTH]);



#endif
