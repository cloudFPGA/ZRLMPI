#ifndef _MPI_H_
#define _MPI_H_


#include <stdint.h>
#include <stdio.h>

#include "zrlmpi_int.hpp"
#include "zrlmpi_common.hpp"

#define MPI_Status uint8_t
#define MPI_Comm   uint8_t
#define MPI_Datatype uint8_t

#define MPI_COMM_WORLD 0
#define MPI_INTEGER 0
#define MPI_FLOAT   1



//void MPI_Init(int* argc, char*** argv);
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


#define MPI_BASE_IP "10.2.200."
#define MPI_PORT 80
#define MPI_SERVICE "80"
#define MPI_CLUSTER_SIZE 2
#define MPI_OWN_RANK 0


#endif
