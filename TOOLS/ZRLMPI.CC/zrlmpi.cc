#!/bin/bash 

HW_TARGET_C=$1/$4/hls/mpi_wrapperv1/src/app_hw.cpp
HW_TARGET_H=$1/$4/hls/mpi_wrapperv1/src/app_hw.hpp

SW_TARGET_C=$1/ZRLMPI/LIB/SW/ZRLMPI/app_sw.cpp
SW_TARGET_H=$1/ZRLMPI/LIB/SW/ZRLMPI/app_sw.hpp

OWN_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ 4 -ne $# ]
then 
  echo "USAGE: $0 path/to/cFp_root_dir path/to/MPI/c-file path/to/MPI/h-file ROLE_DIR"
  echo "This asumes that the the mpi_wrapper in ROLE_DIR will be used"
else 
  make -C $OWN_DIR/unifdef/ 
  if [[ $2 -nt $HW_TARGET_C ]]; then
  echo "./zrlmpi.cc1 $2 $HW_TARGET_C $SW_TARGET_C"
  $OWN_DIR/zrlmpi.cc1 $2 $HW_TARGET_C $SW_TARGET_C
  fi
  if [[ $3 -nt $HW_TARGET_H ]]; then
  echo "./zrlmpi.cc1 $3 $HW_TARGET_H $SW_TARGET_H"
  $OWN_DIR/zrlmpi.cc1 $3 $HW_TARGET_H $SW_TARGET_H
  fi
#./zrlmpi.cc2 $1/FMKU60/TOP/TopFlash
#  ./zrlmpi.cc3 $1/SW/ZRLMPI/
  echo "make of HW and SW currently not implemented"
fi 