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
sed -i "35iZRLMPI_DIR=\$(cFpRootDir)/$1/" $cFpRootDir/Makefile
sed -i "36iCLUSTER_DESCRIPTION=\$(cFpRootDir)/cluster.json" $cFpRootDir/Makefile
sed -i "37iCFP_DESCRIPTION=\$(cFpRootDir)/cFp.json" $cFpRootDir/Makefile

/bin/echo -e "export zrlmpiDir=\"\$rootDir/$1/\"\n\n" >> $cFpRootDir/env/setenv.sh

#TODO: also update cFp.json

# 2. insert new target
#sed -i '40iZrlmpi: assert_env' $cFpRootDir/Makefile
#sed -i '41i\t$(MAKE) -C $(ZRLMPI_DIR) hls' $cFpRootDir/Makefile

# ip cores are now part of the ROLE...so only the SW targets necessary
#sed -i '/#cFa addtional targets/aZrlmpi: assert_env\n\t$(MAKE) -C $(ZRLMPI_DIR) ip\n' $cFpRootDir/Makefile
sed -i '/#cFa addtional targets/ampi_run: assert_env   ## Launches the CPU parts of ZRLMPI\n\t$(MAKE) -C SW/ run\n\nmpi_verify: assert_env  ## Launches the MPI application in openMPI\n\t$(MAKE) -C APP/ run\n\nassert_cluster: \n\t@test -f $(CLUSTER_DESCRIPTION) || ( /bin/echo -e "{\n\"nodes\": {\n\"cpu\" : [0],\n\"fpga\" : [1, 2]\n}\n}" > $(CLUSTER_DESCRIPTION) ; ( [ -d .git/ ] && (git add $(CLUSTER_DESCRIPTION) ) ) ; echo "Please define the cluster setup in $(CLUSTER_DESCRIPTION)" ; exit 1)\n\nupdate_mpi_app: assert_env assert_cluster  ## launches the ZRLMPI cross-compiler\n\t$(ZRLMPI_DIR)/TOOLS/ZRLMPI.CC/zrlmpi.cc  $(cFpRootDir) $(ZRLMPI_DIR) ./APP/*.cpp ./APP/*.hpp $(usedRoleDir) $(CLUSTER_DESCRIPTION) $(CFP_DESCRIPTION)\n\npr_mpi: assert_env update_mpi_app pr  ## cross-compiles the application and builds the PR bitstream\n\nmono_mpi: assert_env update_mpi_app monolithic  ## cross-compiles the application and builds a monolithic bitstream\n\n' $cFpRootDir/Makefile


# 3. modify Role target
# ip cores are now part of the ROLE...
#sed -i 's/\<Role: assert_env\>/& Zrlmpi /' $cFpRootDir/Makefile


# b) create software folder

mkdir -p $cFpRootDir/SW/
cp $cFpRootDir/$1/install/Makefile.SW.template $cFpRootDir/SW/Makefile


# c) copy ROLE files and LINK HLS cores
mkdir -p $usedRoleDir
cp -R $cFpRootDir/$1/templates/ROLE/* $usedRoleDir

#link HLS cores 
mkdir -p $usedRoleDir/hls/
# RELATIVE links!
rel_LIBhls=$(realpath --relative-to=$usedRoleDir/hls/ $cFpRootDir/$1/LIB/HW/hls/)
old_wd=$(pwd)
#ln -s $cFpRootDir/$1/LIB/HW/hls/mpe/ $usedRoleDir/hls/mpe 
cd $usedRoleDir/hls; ln -s $rel_LIBhls/mpe2  ./mpe2
#ln -s $cFpRootDir/$1/LIB/HW/hls/mpi_wrapperv1/ $usedRoleDir/hls/mpi_wrapperv1 
cd $usedRoleDir/hls; ln -s $rel_LIBhls/mpi_wrapperv2  ./mpi_wrapperv2
cd $old_wd

#copy example application
# no!
#cp -R $cFpRootDir/cFDK/APP/hls/triangle_app/ $usedRoleDir/hls/

# d) update .gitignores

/bin/echo -e "\n\n#ZRLMPI specific files\n\
zrlmpi_cpu_app\n\
app_sw.*\n\
app_hw.*\n\n" >> $cFpRootDir/.gitignore

# e) add to git...if existing
[ -d $cFpRootDir/$1/.git/ ] && (git add $usedRoleDir)

# 4. install virual env
cd $cFpRootDir/$1/
pv=$(which python3)
virtualenv py_zrlmpi -p $pv
source py_zrlmpi/bin/activate && pip install -r requirements.txt

# 5. get submodules 

cd $cFpRootDir/$1/
git submodule init
git submodule update

# done?

/bin/echo -e "\n\nThe ZRLMPI tools assume that the MPI application code is in \n  $cFpRootDir/APP/  \nPlease move it there."

exit 0

