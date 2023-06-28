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

#DEPS := $(shell find ./src/ -type f)
DEPS := $(shell find ./src/ )
ifeq ($(usedRoleDir),to-be-defined)
	IP_GUARD=$(usedRole2Dir)/.ip_guard
	IP_GUARD_FORCE=$(usedRole2Dir)/.ip_guard_delete_to_force_tcl_export
else
	IP_GUARD=$(usedRoleDir)/.ip_guard
	IP_GUARD_FORCE=$(usedRoleDir)/.ip_guard_delete_to_force_tcl_export
endif


.PHONY: all csim cosim cosim_view clean assert_env
all: assert_env mpe_prj/solution1/impl/ip


# assert ENVIRONMENT
assert_env:
ifndef usedRoleDir
	@echo "ERROR: usedRoleDir MUST BE DEFINED as environment variable!" 
	@exit 1
endif

mpe_prj/solution1/impl/ip: $(DEPS)
	$(MAKE) clean
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@cat mpe_prj/solution1/syn/report/mpe_main_csynth.rpt
	@#the following is for checking if the AXI4Lite Addresses had changed 
	@#diff ./mpe_prj/solution1/impl/ip/drivers/mpe_main_v1_0/src/xmpe_main_hw.h ./ref/xmpe_main_hw.h
	@#/bin/echo -e "AXI4Lite Addresses checked: OK\n"
	@touch $@
	touch $(IP_GUARD)
	@# to be sure...
	rm -f $(IP_GUARD_FORCE)


csim: clean
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl


cosim:
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl

cosim_view:
	@/bin/echo -e "current_fileset\nopen_wave_database mpe_main.wdb\n" > ./mpe_prj/solution1/sim/verilog/open_wave.tcl
	cd ./mpe_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

clean:
	${RM} -rf mpe_prj
	${RM} vivado*.log

