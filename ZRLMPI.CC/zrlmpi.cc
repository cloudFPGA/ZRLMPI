#!/bin/bash 

if [ 3 -ne $# ]
then 
  #echo "USAGE: $0 path/to/SRA/repo/base path/to/MPI/c-file path/to/MPI/h-file Name-of-Top"
  echo "USAGE: $0 path/to/SRA/repo/base path/to/MPI/c-file path/to/MPI/h-file"
  echo "This asumes that the the mpi_wrapper in RoleMPI will be used as well as TopFlash"
else 
  ./zrlmpi.cc1 $2 $1/FMKU60/ROLE/RoleMPI/hls/mpi_wrapperv1/src/test.cpp $1/ZRLMPI/test.cpp 
  ./zrlmpi.cc1 $3 $1/FMKU60/ROLE/RoleMPI/hls/mpi_wrapperv1/src/test.hpp $1/ZRLMPI/test.hpp 
  ./zrlmpi.cc2 $1/FMKU60/TOP/TopFlash
fi 
