#ifndef JACOBI_TEST_H
#define JACOBI_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>


#define USE_INTEGER


#define DIM 256
//#define WORKER_LINES 64
//#define LDIMX 256
//#define LDIMY 16
//#define PACKETLENGTH 256

//#define DIM 4
//#define LDIMX 4
//#define LDIMY 3
//#define PACKETLENGTH 12

//#define DIM 64
//#define WORKER_LINES 8
//#define ITERATIONS 30
//#define LDIMX 64
//#define LDIMY 4
//#define PACKETLENGTH 256

//#define DIM 16
//#define WORKER_LINES 8
//#define LDIMX 16
//#define LDIMY 8
//#define PACKETLENGTH 1344

//#define DIM 1024
//#define WORKER_LINES 128

//#define DIM 512
//#define WORKER_LINES 128

//#define PACKETLENGTH 256
//#define PACKETLENGTH 512
//#define PACKETS_PER_LINE (DIM/PACKETLENGTH)


#define CORNER_VALUE_INT 128

#define DATA_CHANNEL_TAG 1
#define CMD_CHANNEL_TAG 2


#define ITERATIONS 2


int main( int argc, char **argv );

#endif
