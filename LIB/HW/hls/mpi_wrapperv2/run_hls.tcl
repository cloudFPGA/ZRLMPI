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

# *****************************************************************************
# *                            cloudFPGA
# *----------------------------------------------------------------------------
# * Created : Jun 2017
# * Authors : Burkhard Ringlein
# * 
# * Description : A Tcl script for the HLS batch syhthesis of the "Castor" SMC
# *   process used by the SHELL of a cloudFPGA module.
# *   project.
# * 
# * Synopsis : vivado_hls -f <this_file>
# *
# *
# * Reference documents:
# *  - UG902 / Ch.4 / High-Level Synthesis Reference Guide.
# *
# *-----------------------------------------------------------------------------
# * Modification History:
# ******************************************************************************

# User defined settings
#-------------------------------------------------
#set appName        "jacobi2d"
set appName        "mpi_wrapper"
set solutionName   "solution1"
set xilPartName    "xcku060-ffva1156-2-i"
set projectName    "${appName}v2"

set ipName         ${projectName}
set ipDisplayName  "Jacobi2D filter with MPI"
set ipDescription  "Application for cloudFPGA"
set ipVendor       "IBM"
set ipLibrary      "hls"
set ipVersion      "1.0"
set ipPkgFormat    "ip_catalog"

# Set Project Environment Variables  
#-------------------------------------------------
set currDir      [pwd]
set srcDir       ${currDir}/src
set tbDir        ${currDir}/tb
#set implDir      ${currDir}/${appName}_prj/${solutionName}/impl/ip 
#set repoDir      ${currDir}/../../ip


# Get targets out of env  
#-------------------------------------------------

set hlsSim $env(hlsSim)
set hlsCoSim $env(hlsCoSim)

# Open and Setup Project
#-------------------------------------------------
open_project  ${projectName}_prj
#set_top       ${appName}_main
set_top       mpi_wrapper


# for Debug
add_files     ${srcDir}/app_hw.hpp -cflags "-DCOSIM"
add_files     ${srcDir}/app_hw.cpp -cflags "-DCOSIM"
#add_files     ${srcDir}/${appName}.hpp -cflags "-DCOSIM"
#add_files     ${srcDir}/${appName}.cpp -cflags "-DCOSIM"
add_files     ${srcDir}/ZRLMPI.hpp -cflags "-DCOSIM"
add_files     ${srcDir}/ZRLMPI.cpp -cflags "-DCOSIM"
add_files     ${srcDir}/zrlmpi_common.hpp
add_files     ${srcDir}/zrlmpi_common.cpp
add_files     ${srcDir}/zrlmpi_int.hpp

#for DEBUG flag 
#add_files -tb src/smc.cpp -cflags "-DDEBUG"
add_files -tb tb/tb_${appName}.cpp 


open_solution ${solutionName}

set_part      ${xilPartName}
create_clock -period 6.4 -name default

source ${srcDir}/app_hw_directives.tcl
config_interface -m_axi_addr64

# Run C Simulation and Synthesis
#-------------------------------------------------

if { $hlsSim} { 
  csim_design -compiler gcc -clean
} else {

  csynth_design
  
  if { $hlsCoSim} {
    cosim_design -compiler gcc -trace_level all 
  } else {
  
  # Export RTL
  #-------------------------------------------------
    export_design -rtl vhdl -format ${ipPkgFormat} -library ${ipLibrary} -display_name ${ipDisplayName} -description ${ipDescription} -vendor ${ipVendor} -version ${ipVersion}
  }
}

exit

