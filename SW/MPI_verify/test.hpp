#ifndef JACOBI_TEST_H
#define JACOBI_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#define USE_INTEGER

#define DIM 256
#define LDIMX 256
#define LDIMY 16

#define PACKETLENGTH 256
#define CORNER_VALUE_INT 128

#define DATA_CHANNEL_TAG 1 
#define CMD_CHANNEL_TAG 2


int main( int argc, char **argv );

#endif
