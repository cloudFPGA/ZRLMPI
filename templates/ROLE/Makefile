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
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Makefile to build the ROLE for a cloudFPGA-project (cFp)
#  * 
#  *     Synopsis:
#  *       make [target]  (use 'make help' to list the targets)
#  * 



CLEAN_TYPES = *.log *.jou *.str *.time


.PHONY: all clean hls_cores assert_env help

## ZRLMPI: app is always changing, so to be sure to rebuild it 
.PHONY: ip

all: assert_env Role_$(cFpSRAtype)_OOC.dcp ## builds the Out-Of-Contect (OOC) Design-Checkpoint (dcp) for this Role

# assert ENVIRONMENT
assert_env: 
	@../../cFDK/SRA/LIB/bash/assert_envs.sh

Role_$(cFpSRAtype)_OOC.dcp: ./hdl ./tcl/run_pr.tcl ./tcl/xpr_settings_role.tcl | ip  ## builds the Out-Of-Contect (OOC) Design-Checkpoint (dcp) for this Role (default)
	cd ./tcl/; vivado -mode batch -source run_pr.tcl -notrace -log run_pr.log -tclargs -force
	@touch $@  # to update all timestamps...

#hls_cores: ./.ip_guard_delete_to_force_tcl_export
hls_cores:
	touch ./.ip_guard_delete_to_force_tcl_export
	@$(MAKE) -C ./hls/

hls: assert_env hls_cores  ## starts building all HLS cores

# We need ./.ip_guard to be touched by the hls ip core makefiles, because HLS_DEPS doesn't work. i
# HLS_DEPS get's evaluated BEFORE the hls target get's executed, so if a hls core doesn't exist completly (e.g. after a clean)
# the create_ip_cores.tcl will not be started. 
# DIRECOTRIES are better order-only-prerequisities
ip: ./tcl/create_ip_cores.tcl  ./.ip_guard | ./.ip_guard_delete_to_force_tcl_export hls ./ip/ip_user_files   ## export ROLE HLS cores to Vivado IP direcotry
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log
	@touch $@
	@echo ------- DONE -------------------------------------

.ip_guard:
	@touch $@

# this is REQUIRED by the ZRLMPI cFa
.ip_guard_delete_to_force_tcl_export:
	@touch $@

# Create IP directory if it does not exist
./ip/ip_user_files:
	@echo -- Creating ip/ip_user_files directory -----------
	mkdir -p $@


clean: ## Cleans this ROLE
	rm -rf ./ip ./xpr
	rm -rf $(CLEAN_TYPES)
	rm -rf ./tcl/*.log ./tcl/*.jou
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


