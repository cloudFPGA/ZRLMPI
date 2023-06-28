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

#ifndef JACOBI_TEST_H
#define JACOBI_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include "ZRLMPI.hpp"

#define USE_INTEGER


//#define DIM 256
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

#define DIM 16
#define WORKER_LINES 8
#define ITERATIONS 1
//#define LDIMX 16
//#define LDIMY 8
//#define PACKETLENGTH 1344


#define CORNER_VALUE_INT 128

#define DATA_CHANNEL_TAG 1 
#define CMD_CHANNEL_TAG 2

using namespace hls;

int app_main(
    // ----- MPI_Interface -----
    stream<MPI_Interface> *soMPIif,
    stream<Axis<8> > *soMPI_data,
    stream<Axis<8> > *siMPI_data
    );

#endif
