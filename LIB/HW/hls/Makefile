# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Aug 2019
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that invokes all the HLS synthesis batches located 
# *   in the current sub-directories.
# *
# ******************************************************************************


.PHONY: all clean mpe

all: mpe


*:
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $@

clean:
	rm -rf ./*/*_prj
	rm -rf ./*/*/*_prj
	rm -rf ./*/*/*/*_prj

