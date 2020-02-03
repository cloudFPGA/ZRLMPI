#!/bin/bash 
#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Dec 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Main bash script of ZRLMPIcc
#  *
#  *



HW_TARGET_C=$5/hls/mpi_wrapperv1/src/app_hw.cpp
HW_TARGET_H=$5/hls/mpi_wrapperv1/src/app_hw.hpp

SW_TARGET_C=$1/SW/app_sw.cpp
SW_TARGET_H=$1/SW/app_sw.hpp

OWN_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

PY_ENV=$2/py_zrlmpi/

if [ 6 -ne $# ]
then 
  echo "USAGE: $0 path/to/cFp_root_dir path/to/ZRLMPI_dir path/to/MPI/c-file path/to/MPI/h-file path/to/ROLE_DIR path/to/cluster/description_json"
  echo "This asumes that the the mpi_wrapper in ROLE_DIR will be used"
else 
  make -C $OWN_DIR/unifdef/ 
  if [[ $3 -nt $HW_TARGET_C ]] || [[ $4 -nt $HW_TARGET_H ]] \
  || [[ $3 -nt $SW_TARGET_C ]] || [[ $4 -nt $SW_TARGET_H ]] \
  || [[ $6 -nt $HW_TARGET_C ]] || [[ $6 -nt $HW_TARGET_H ]]; then
  # the json only influences the HW targets...
  echo "$OWN_DIR/zrlmpi.cc1 $PY_ENV $3 $4 $HW_TARGET_C $HW_TARGET_H $SW_TARGET_C $SW_TARGET_H $6"
  $OWN_DIR/zrlmpi.cc1 $PY_ENV $3 $4 $HW_TARGET_C $HW_TARGET_H $SW_TARGET_C $SW_TARGET_H $6
  fi
  ./zrlmpi.cc2 $1
  ./zrlmpi.cc3 $1
fi 
