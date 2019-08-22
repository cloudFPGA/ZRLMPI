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
# we update the present Makefile (alternative: copy Makefile.template from install folder)

# 1. add ZRLMPI Dir
sed -i "19iZRLMPI_DIR=\$(cFpRootDir)/$1/" $cFpRootDir/Makefile

# 2. insert new target
#sed -i '40iZrlmpi: assert_env' $cFpRootDir/Makefile
#sed -i '41i\t$(MAKE) -C $(ZRLMPI_DIR) hls' $cFpRootDir/Makefile
sed -i '/#cFa addtional targets/aZrlmpi: assert_env\n\t$(MAKE) -C $(ZRLMPI_DIR) hls\n' $cFpRootDir/Makefile

# 3. modify Role target
sed -i 's/\<Role: assert_env\>/& Zrlmpi /' $cFpRootDir/Makefile


exit 0

