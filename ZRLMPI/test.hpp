#ifndef JACOBI_TEST_H
#define JACOBI_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include "MPI.hpp"

#define USE_INTEGER
#define DIM 4
#define LDIMX 4
#define LDIMY 3 // = DIM/2 + 1

#define PACKETLENGTH 256
#define CORNER_VALUE_INT 128

#define DATA_CHANNEL_TAG 1 
#define CMD_CHANNEL_TAG 2


int app_main();

#endif
