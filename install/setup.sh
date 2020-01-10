#!/bin/bash 

#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Aug 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Setup script for this cloudFPGA addon (cFa)
#  *


# source ../env/setenv.sh is done by cFBuild

if [ $# -ne 1 ]; then
	echo "Illegal number of parameters"
	echo "USAGE: $0 <name-of-addon>"
	exit 1
fi

echo "Installing $1 into $cFpRootDir ..."

#use absolute pathes, just to be sure...

# a) we update the present Makefile (alternative: copy Makefile.template from install folder)
# 1. add ZRLMPI Dir and update env
sed -i "19iZRLMPI_DIR=\$(cFpRootDir)/$1/" $cFpRootDir/Makefile

/bin/echo -e "export zrlmpiDir=\"\$rootDir/$1/\"\n\n" >> $cFpRootDir/env/setenv.sh

#TODO: also update cFp.json

# 2. insert new target
#sed -i '40iZrlmpi: assert_env' $cFpRootDir/Makefile
#sed -i '41i\t$(MAKE) -C $(ZRLMPI_DIR) hls' $cFpRootDir/Makefile

# ip cores are now part of the ROLE...so only the SW targets necessary
#sed -i '/#cFa addtional targets/aZrlmpi: assert_env\n\t$(MAKE) -C $(ZRLMPI_DIR) ip\n' $cFpRootDir/Makefile
sed -i '/#cFa addtional targets/ampi_run: assert_env   ## Launches the CPU parts of ZRLMPI\n\t$(MAKE) -C SW/ run\n\n\
mpi_verify: assert_env  ## Launches the MPI application in openMPI\n\t$(MAKE) -C APP/ make run\n\n\
update_mpi_app: assert_env   ## launches the ZRLMPI cross-compiler\n\t$(ZRLMPI_DIR)/TOOLS/ZRLMPI.CC/zrlmpi.cc  $(cFpRootDir) ./APP/*.cpp ./APP/*.hpp $(usedRoleDir)\n\n\
pr_mpi: assert_env update_mpi_app pr  ## cross-compiles the application and builds the PR bitstream\n\n\
\n\n\
mono_mpi: assert_env update_mpi_app monolithic  ## cross-compiles the application and builds a monolithic bitstream\n\n' $cFpRootDir/Makefile


# 3. modify Role target
# ip cores are now part of the ROLE...
#sed -i 's/\<Role: assert_env\>/& Zrlmpi /' $cFpRootDir/Makefile


# b) create software folder

mkdir -p $cFpRootDir/SW/
cp $cFpRootDir/$1/install/Makefile.SW.template $cFpRootDir/SW/Makefile


# c) copy ROLE files and LINK HLS cores

mkdir -p $cFpRoleDir
cp -R $cFpRootDir/$1/templates/ROLE/* $cFpRoleDir

ln -s $cFpRootDir/$1/LIB/HW/hls/mpe/ $cFpRoleDir/hls/mpe 
ln -s $cFpRootDir/$1/LIB/HW/hls/mpi_wrapperv1/ $cFpRoleDir/hls/mpi_wrapperv1 

# d) update .gitignores

/bin/echo -e "\n\n#ZRLMPI specific files\n\
zrlmpi_cpu_app\n\
app_sw.*\n\
app_hw.*\n\n" >> $cFpRootDir/.gitignore



#TODO:
# 4. create bash-script for ZRLMPI.CC and ZRLMPI.RUN in cFp?
# 5. create APP folder

/bin/echo -e "\n\nThe ZRLMPI tools assume that the MPI application code is in \n  $cFpRootDir/APP/  \nPlease move it there."

exit 0

