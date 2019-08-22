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
# 1. add ZRLMPI Dir
sed -i "19iZRLMPI_DIR=\$(cFpRootDir)/$1/" $cFpRootDir/Makefile

# 2. insert new target
#sed -i '40iZrlmpi: assert_env' $cFpRootDir/Makefile
#sed -i '41i\t$(MAKE) -C $(ZRLMPI_DIR) hls' $cFpRootDir/Makefile
sed -i '/#cFa addtional targets/aZrlmpi: assert_env\n\t$(MAKE) -C $(ZRLMPI_DIR) ip\n' $cFpRootDir/Makefile

# 3. modify Role target
sed -i 's/\<Role: assert_env\>/& Zrlmpi /' $cFpRootDir/Makefile


# b) create software folder

mkdir -p $cFpRootDir/SW/
cp $cFpRootDir/$1/install/Makefile.SW.template $cFpRootDir/SW/Makefile


# c) copy ROLE files

# TODO 
#echo -e "\n\tINFO: Please find the ROLE sources in $1/ROLE/ and copy the necessary parts.\n"
#mkdir -p $cFpRootDir/ROLE/
#cp -R $cFpRootDir/$1/ROLE/* $cFpRootDir/ROLE/
#
#echo -e '#ZRLMPI updates\nexport roleName1="RoleMPI"\nexport roleName2="RoleMPI_V2"\n\n' >> $cFpRootDir/env/setenv.sh

#TODO:
# 1. copy mpi_wrapper into Role, all other files (hdl, tcl, Makefiles etc. too, if necessary)
# 2. copy makefile to SW 
# 3. create Makefile for ZRLMPI.CC
# 4. create bash-script for ZRLMPI.CC and ZRLMPI.RUN in cFp?
# 5. create APP folder
# mpi verify?

exit 0

