#!/bin/bash
# /*******************************************************************************
#  * Copyright 2018 -- 2023 IBM Corporation
#  *
#  * Licensed under the Apache License, Version 2.0 (the "License");
#  * you may not use this file except in compliance with the License.
#  * You may obtain a copy of the License at
#  *
#  *     http://www.apache.org/licenses/LICENSE-2.0
#  *
#  * Unless required by applicable law or agreed to in writing, software
#  * distributed under the License is distributed on an "AS IS" BASIS,
#  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  * See the License for the specific language governing permissions and
#  * limitations under the License.
# *******************************************************************************/

#  *
#  *                       cloudFPGA
#  *    =============================================
#  *     Created: Dec 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Main bash script of ZRLMPIcc
#  *
#  *



HW_TARGET_C=$5/hls/mpi_wrapperv2/src/app_hw.cpp
HW_TARGET_H=$5/hls/mpi_wrapperv2/src/app_hw.hpp
HW_TARGET_TCL=$5/hls/mpi_wrapperv2/src/app_hw_directives.tcl

ZRLMPI_TEMPLATE_C=$5/hls/mpi_wrapperv2/src/ZRLMPI_temp.cpp
ZRLMPI_TARGET_C=$5/hls/mpi_wrapperv2/src/ZRLMPI.cpp

ROLE_TEMPLATE_VHDL=$2/templates/Role_template.vhdl
ROLE_TARGET_VHDL=$5/hdl/Role.vhdl

SW_TARGET_C=$1/SW/app_sw.cpp
SW_TARGET_H=$1/SW/app_sw.hpp

OWN_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

PY_ENV=$2/py_zrlmpi/

if [ 7 -ne $# ]
then 
  echo "USAGE: $0 path/to/cFp_root_dir path/to/ZRLMPI_dir path/to/MPI/c-file path/to/MPI/h-file path/to/ROLE_DIR path/to/cluster/description_json path/to/cFp_json"
  echo "This asumes that the the mpi_wrapper in ROLE_DIR will be used"
else 
  make -C $OWN_DIR/unifdef/
  # disabling timestamp check for now..
  #if [[ $3 -nt $HW_TARGET_C ]] || [[ $4 -nt $HW_TARGET_H ]] \
  #  || [[ $3 -nt $SW_TARGET_C ]] || [[ $4 -nt $SW_TARGET_H ]] \
  #|| [[ $6 -nt $HW_TARGET_C ]] || [[ $6 -nt $HW_TARGET_H ]]; then
  #  # the json only influences the HW targets...
    echo "$OWN_DIR/zrlmpi.cc1 $PY_ENV $3 $4 $HW_TARGET_C $HW_TARGET_H $SW_TARGET_C $SW_TARGET_H $6"
    $OWN_DIR/zrlmpi.cc1 $PY_ENV $3 $4 $HW_TARGET_C $HW_TARGET_H $HW_TARGET_TCL $SW_TARGET_C $SW_TARGET_H $6 $7 $ZRLMPI_TEMPLATE_C $ZRLMPI_TARGET_C $ROLE_TEMPLATE_VHDL $ROLE_TARGET_VHDL
    ret=$?
 #else
 #  ret=0
 #fi
  if [[ $ret -eq 0 ]]; then 
    $OWN_DIR/zrlmpi.cc2 $1
    $OWN_DIR/zrlmpi.cc3 $1
  else 
    echo "ERROR during cross-compilation; stopping..."
  fi
fi

