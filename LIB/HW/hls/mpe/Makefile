

#DEPS := $(shell find ./src/ -type f)
DEPS := $(shell find ./src/ )
IP_GUARD=$(usedRoleDir)/.ip_guard


.PHONY: all csim cosim cosim_view clean assert_env
all: mpe_prj/solution1/impl/ip


# assert ENVIRONMENT
assert_env:
ifndef usedRoleDir
	@echo "ERROR: usedRoleDir MUST BE DEFINED as environment variable!" 
	@exit 1
endif

mpe_prj/solution1/impl/ip: assert_env $(DEPS)
	$(MAKE) clean
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@#the following is for checking if the AXI4Lite Addresses had changed 
	@#diff ./mpe_prj/solution1/impl/ip/drivers/mpe_main_v1_0/src/xmpe_main_hw.h ./ref/xmpe_main_hw.h
	@#/bin/echo -e "AXI4Lite Addresses checked: OK\n"
	@touch $@
	@echo "touch $(IP_GUARD)"
	@touch $(IP_GUARD)


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
