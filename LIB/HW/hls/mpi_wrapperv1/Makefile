# Name of the HLS core to build
#ipName = jacobi2dv0
appName =mpi_wrapper
ipName =${appName}v1
DIM=16

#DEPS := $(shell find ./src/ -type f)
DEPS := $(shell find ./src/ )
IP_GUARD=$(usedRoleDir).ip_guard
IP_GUARD_FORCE=$(usedRoleDir).ip_guard_delete_to_force_tcl_export

.PHONY: all csim cosim cosim_view assert_env
all: assert_env ${ipName}_prj/solution1/impl/ip

# assert ENVIRONMENT
assert_env:
ifndef usedRoleDir
	@echo "ERROR: usedRoleDir MUST BE DEFINED as environment variable!" 
	@exit 1
endif

${ipName}_prj/solution1/impl/ip: $(DEPS)
	@#rm -rf $@
	rm -rf ${ipName}_prj
	export hlsSim=0; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	@#touch ${ipName}_prj
	@touch $@
	touch $(IP_GUARD)
	@# to be sure...
	rm -f $(IP_GUARD_FORCE)

csim:
	@/bin/echo -e "---------------\n\tThis simulation works only with *one* iteration with DIM $(DIM)!\n\t(so it expects one recv and one send from the app)\n---------------\n"
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl


cosim:
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl

cosim_view:
	@/bin/echo -e "current_fileset\nopen_wave_database $(appName).wdb\n" > ./$(ipName)_prj/solution1/sim/verilog/open_wave.tcl
	cd ./$(ipName)_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

clean:
	rm -rf ${ipName}_prj
	rm vivado*.log


