#ifndef JACOBI_COMMON_H
#define JACOBI_COMMON_H

#include <stdlib.h>
#include <stdio.h>

#define USE_INTEGER

#define DIM 256
#define LDIMX 256
#define LDIMY 16

#define PACKETLENGTH 256
#define CORNER_VALUE_INT 128

#define DATA_CHANNEL_TAG 1 
#define CMD_CHANNEL_TAG 2

#ifdef USE_INTEGER
void print_array(const int *A, size_t width, size_t height)
#else
void print_array(const double *A, size_t width, size_t height)
#endif
{
  printf("\n");
  for(size_t i = 0; i < height; ++i)
  {
    for(size_t j = 0; j < width; ++j)
    {
#ifdef USE_INTEGER
      printf("%d ", A[i * width + j]);
#else
      printf("%.2f ", A[i * width + j]);
#endif
    }
    printf("\n");
  }
  printf("\n");
}


#endif
