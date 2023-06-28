/*******************************************************************************
 * Copyright 2018 -- 2023 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*******************************************************************************/

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

#define VECTOR_DIM 3

//#define NR_VECTORS 50
//#define NR_VECTORS 10000
#define NR_VECTORS 1000000


#define K_STATIC 512
#define MAX_CENTROIDS 512
//#define SCALE_FACTOR 5
//#define SCALE_FACTOR 10

//#define NORM_LOWER_LIMIT 100000
//#define NORM_LOWER_LIMIT (100000/SCALE_FACTOR)
//#define NORM_LOWER_LIMIT (INT_MAX/10000)
#define NORM_LOWER_LIMIT 10

//#define MAX_ITERATIONS 2048
//#define MAX_ITERATIONS 16
#define MAX_ITERATIONS 128


//#define MAX_ITERATIONS 2
//#define PRINTALL


int main( int argc, char **argv );



#endif

