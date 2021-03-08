#ifndef _MPI_H_
#define _MPI_H_


#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zrlmpi_int.hpp"
#ifdef _ZRLMPI_APP_INCLUDED_
#include "zrlmpi_common.hpp"
#endif

#define MPI_Status uint8_t
#define MPI_Comm   uint8_t
#define MPI_Datatype uint8_t

#define MPI_COMM_WORLD 0
#define MPI_INTEGER 0
#define MPI_FLOAT   1


//#define my_memcpy memcpy

void MPI_Init(int* argc, char*** argv);
void MPI_Init();
void MPI_Comm_rank(MPI_Comm communicator, int* rank);
void MPI_Comm_size( MPI_Comm communicator, int* size);


void MPI_Send(
    int* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator);

void MPI_Recv(
    int* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status);

void MPI_Finalize();

//void MPI_Barrier(MPI_Comm communicator);


//#define MPI_BASE_IP "10.12.200."
#define MPI_PORT 2718
#define MPI_SERVICE "2718"
#define MPI_CLUSTER_SIZE_MAX 128
#define UDP_HEADER_SIZE_BYTES 45

//#define CUSTOM_MTU 1402 //VPN
//#define CUSTOM_MTU 1000 //VPN2
//#define CUSTOM_MTU 1500 //ZC2
#define CUSTOM_MTU 1416 //ZYC2

//#define KVM_CORRECTION //defined in Makefile
#define KVM_CORRECTION_US 200
#define KVM_NETWORK_LOSS 80000 //so probability is 1/KVM_NETWORK_LOSS

#define USE_PROTO_TIMEOUT

#define USE_DRAM_AWARE_TRANSMISSION
#define DRAM_TRANSMISSION_THRESHOLD_WORDS 32768


//forward declaration
#ifndef _ZRLMPI_APP_INCLUDED_
int app_main(int argc, char **argv);
#endif

#endif
