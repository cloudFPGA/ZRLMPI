#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Makefile to build the ROLE for a cloudFPGA-project (cFp)
#  * 
#  *     Synopsis:
#  *       make [target]  (use 'make help' to list the targets)
#  * 


HLS_DEPS := $(shell find ./hls/*/*_prj/solution1/impl/ip -maxdepth 0 -type d)


CLEAN_TYPES = *.log *.jou *.str *.time


.PHONY: all clean hls_cores assert_env help


all: assert_env Role_$(cFpSRAtype)_OOC.dcp ## builds the Out-Of-Contect (OOC) Design-Checkpoint (dcp) for this Role

# assert ENVIRONMENT
assert_env: 
	@../../cFDK/SRA/LIB/bash/assert_envs.sh

Role_$(cFpSRAtype)_OOC.dcp: ./hdl ./ip ./tcl  ./.ip_guard ## builds the Out-Of-Contect (OOC) Design-Checkpoint (dcp) for this Role (default)
	cd ./tcl/; vivado -mode batch -source run_pr.tcl -notrace -log run_pr.log -tclargs -force

hls_cores:
	@$(MAKE) -C ./hls/

hls: assert_env hls_cores  ## starts building all HLS cores

# We need ./.ip_guard to be touched by the hls ip core makefiles, because HLS_DEPS doesn't work. i
# HLS_DEPS get's evaluated BEFORE the hls target get's executed, so if a hls core doesn't exist completly (e.g. after a clean)
# the create_ip_cores.tcl will not be started. 
# TODO: $(HLS_DEPS) obsolete?
ip: hls ./tcl/create_ip_cores.tcl $(HLS_DEPS) ./ip/ip_user_files ./.ip_guard  ## export ROLE HLS cores to Vivado IP direcotry
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log 
	@echo ------- DONE ------------------------------------- 

.ip_guard: 
	@touch $@

# Create IP directory if it does not exist
./ip/ip_user_files:
	@echo -- Creating ip/ip_user_files directory -----------
	mkdir -p $@


clean: ## Cleans this ROLE
	rm -rf ./ip ./xpr
	rm -rf $(CLEAN_TYPES)
	rm -rf ./tcl/$(CLEAN_TYPES)
	$(MAKE) -C ./hls clean
	rm -rf Role*_OOC.dcp


print-%: ## A little make receipt to print a variable (usage example --> 'make print-SHELL_DIR')
	@echo $* = $($*)

help: ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@cat ${MAKEFILE_LIST}  | egrep '^(.+)\:(.+)##\ (.+)' |  column -s ':#' |  sed 's/:.*#/||/' |  column -s '||' -t | sed -e 's/^/    /' 
	@echo

