#ifndef _MPI_H_
#define _MPI_H_


#include <stdint.h>
#include <stdio.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>

#include "zrlmpi_common.hpp"

using namespace hls;

#define WAIT_CYCLES 2  //It looks like, this MUST BE AT LEAST 2

//Display0
#define RECV_CNT_SHIFT 8
#define SEND_CNT_SHIFT 4
#define AP_DONE_SHIFT 12
#define AP_INIT_SHIFT 14
#define MEM_ERR_SHIFT 15

#define RECV_WRITE_INFO 0
#define RECV_READ_DATA 1
#define RECV_FINISH 2
#define RECV_DONE 3
#define WrapperRecvState uint8_t

#define SEND_WRITE_INFO 0
#define SEND_WRITE_DATA 1
#define SEND_FINISH 2
#define SEND_DONE 3
#define WrapperSendState uint8_t



/*
 * A generic unsigned AXI4-Stream interface used all over the cloudFPGA place.
 */
template<int D>
struct Axis {
  ap_uint<D>       tdata;
  ap_uint<(D+7)/8> tkeep;
  ap_uint<1>       tlast;
  Axis() {}
  Axis(ap_uint<D> single_data) : tdata((ap_uint<D>)single_data), tkeep(1), tlast(1) {}
};

//#define MPI_SEND_INT 0
//#define MPI_RECV_INT 1
//#define MPI_SEND_FLOAT 2
//#define MPI_RECV_FLOAT 3
//#define MPI_BARRIER 4
//#define mpiCall uint8_t
//
///*
// * MPI-F Interface
// */
//struct MPI_Interface {
//  ap_uint<8>     mpi_call;
//  ap_uint<32>    count;
//  ap_uint<32>    rank;
//  MPI_Interface() {}
//};
//

#define MPI_Status uint8_t
#define MPI_Comm   uint8_t
#define MPI_Datatype uint8_t

#define MPI_COMM_WORLD 0
#define MPI_INTEGER 0
#define MPI_FLOAT   1


void my_perror(const char *s);
void my_exit(int status);
void my_free(void *ignore);

void MPI_Init();
void MPI_Init(void *a, void *b);
void MPI_Comm_rank(MPI_Comm communicator, int* rank);
void MPI_Comm_size( MPI_Comm communicator, int* size);


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
    MPI_Comm communicator);


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
    MPI_Status* status);


void MPI_Finalize();

#define DRAM_BYTES_PER_LINE 64
#define ZRLMPI_DRAM_SIZE_LINES 134217728 //8GB
//#define ZRLMPI_DRAM_SIZE_LINES 100663296 //6GB
#define BOFDRAM_LINE_RESERVATION 2

void mpi_wrapper(
    // ----- FROM FMC -----
    ap_uint<32> role_rank_arg,
    ap_uint<32> cluster_size_arg,
    // ----- TO FMC ------
    ap_uint<16> *MMIO_out,
    // ----- MPI_Interface -----
    stream<MPI_Interface> *soMPIif,
    stream<MPI_Feedback> *siMPIFeB,
    stream<Axis<64> > *soMPI_data,
    stream<Axis<64> > *siMPI_data,
    // ----- DRAM -----
    ap_uint<512> boFdram[ZRLMPI_DRAM_SIZE_LINES]
    );


#endif
