# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Sep 2018
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that invokes all the HLS synthesis batches located 
# *   in the current sub-directories.
# *
# ******************************************************************************

.PHONY: all clean triangle_app mpe2 mpi_wrapperv2

all: mpe2 mpi_wrapperv2 triangle_app


*:
	@#echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $@

clean:
	rm -rf ./*/*_prj
