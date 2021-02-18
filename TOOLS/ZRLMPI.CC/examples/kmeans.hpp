#ifndef _KMEANS_TEST_H_
#define _KMEANS_TEST_H_


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>
#include <mpi.h>
#include <limits.h>


#define DATA_CHANNEL_TAG 1
#define CMD_CHANNEL_TAG 2


//#define NR_VECTORS 50
#define NR_VECTORS 10000


#define K_STATIC 512
#define MAX_CENTROIDS 512
#define SCALE_FACTOR (1)

//#define NORM_LOWER_LIMIT 100000
//#define NORM_LOWER_LIMIT (100000/SCALE_FACTOR)
//#define NORM_LOWER_LIMIT (INT_MAX/10000)
#define NORM_LOWER_LIMIT 10000

//#define MAX_ITERATIONS 2048
//#define MAX_ITERATIONS 16
#define MAX_ITERATIONS 128


//#define PRINTALL


int main( int argc, char **argv );



#endif

