# Name of the HLS core to build
#ipName = jacobi2dv0
appName =mpi_wrapper
ipName =${appName}v1

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
	@cat ${ipName}_prj/solution1/syn/report/${appName}_csynth.rpt
	@#touch ${ipName}_prj
	@touch $@
	touch $(IP_GUARD)
	@# to be sure...
	rm -f $(IP_GUARD_FORCE)

csim:
	@/bin/echo -e "---------------\n\tThis simulation works only with *one* iteration with DIM 16!\n\tSo the test_app.cpp/hpp will be used\n\t(to use the actual app_hw.cpp use 'make csim_orig`)\n---------------\n"
	cp src/app_hw.cpp ./backup_app_hw.cpp
	cp src/app_hw.hpp ./backup_app_hw.hpp
	cp src/test_app.cpp src/app_hw.cpp
	cp src/test_app.hpp src/app_hw.hpp
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl
	cp src/backup_app_hw.cpp src/app_hw.cpp
	cp src/backup_app_hw.hpp src/app_hw.hpp

csim_orig:
	@/bin/echo -e "---------------\n\tThis simulation works only with *one* iteration with DIM 16!\n\t(so it expects one recv and one send from the app)\n---------------\n"
	export hlsSim=1; export hlsCoSim=0; vivado_hls -f run_hls.tcl

cosim:
	export hlsSim=0; export hlsCoSim=1; vivado_hls -f run_hls.tcl

cosim_view:
	@/bin/echo -e "current_fileset\nopen_wave_database $(appName).wdb\n" > ./$(ipName)_prj/solution1/sim/verilog/open_wave.tcl
	cd ./$(ipName)_prj/solution1/sim/verilog/; vivado -source open_wave.tcl

clean:
	rm -rf ${ipName}_prj
	rm -f vivado*.log hs*.log


