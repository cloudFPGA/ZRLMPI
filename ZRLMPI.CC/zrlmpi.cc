#!/bin/bash 

HW_TARGET_C=$1/FMKU60/ROLE/RoleMPI/hls/mpi_wrapperv1/src/test.cpp
HW_TARGET_H=$1/FMKU60/ROLE/RoleMPI/hls/mpi_wrapperv1/src/test.hpp

SW_TARGET_C=$1/SW/ZRLMPI/test.cpp
SW_TARGET_H=$1/SW/ZRLMPI/test.hpp

if [ 3 -ne $# ]
then 
  #echo "USAGE: $0 path/to/SRA/repo/base path/to/MPI/c-file path/to/MPI/h-file Name-of-Top"
  echo "USAGE: $0 path/to/SRA/repo/base path/to/MPI/c-file path/to/MPI/h-file"
  echo "This asumes that the the mpi_wrapper in RoleMPI will be used as well as TopFlash"
else 
  if [[ $2 -nt $HW_TARGET_C ]]; then
  echo "./zrlmpi.cc1 $2 $HW_TARGET_C $SW_TARGET_C"
  ./zrlmpi.cc1 $2 $HW_TARGET_C $SW_TARGET_C
  fi
  if [[ $3 -nt $HW_TARGET_H ]]; then
  echo "./zrlmpi.cc1 $3 $HW_TARGET_H $SW_TARGET_H"
  ./zrlmpi.cc1 $3 $HW_TARGET_H $SW_TARGET_H
  fi
  ./zrlmpi.cc2 $1/FMKU60/TOP/TopFlash
  ./zrlmpi.cc3 $1/SW/ZRLMPI/
fi 
