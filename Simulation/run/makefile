###############################################################################
# Makefile to run verilog simulations, using Icarus Verilog
# C.Y. Chen <chingyuanchen@g.ntu.edu.tw>
###############################################################################
SHELL := bash

#-------------------------------------------------------------------------------
# default variables
#-------------------------------------------------------------------------------
TB := tb_core 
MEM_SIZE := 4096
DSEG_SIZE := 1024 
TIMEOUT := 10000
DUMPVCD := 0
STIMULI := 0
SYN := 0 

#-------------------------------------------------------------------------------
# variables
# must match the name on create_filelist script
#-------------------------------------------------------------------------------
.FILE_LIST     = out/filelist.prj
.MEM_FILE_NAME = ../out/pmem.mem

#-------------------------------------------------------------------------------
# Test folders
#-------------------------------------------------------------------------------
.ASM_TESTS_FOLDER=../tests/asm
.ASM_TESTS=$(shell ls -1 $(.ASM_TESTS_FOLDER))

.C_TESTS_FOLDER=../tests/c
.C_TESTS=$(shell ls -1 $(.C_TESTS_FOLDER))


#-------------------------------------------------------------------------------
# targets
#-------------------------------------------------------------------------------
.PHONY: default clean list_asm_tests list_c_tests

#-------------------------------------------------------------------------------
help:
	@../scripts/help_screen.sh

#-------------------------------------------------------------------------------
check:	.Filelist
	@../scripts/check_verilog.sh $(.FILE_LIST)

#-------------------------------------------------------------------------------
.check-%:
	@if [ "${${*}}" == "" ]; then \
		echo -e ""; \
		echo -e "Makefile: ERROR: Environment variable $* not set"; \
		echo -e ""; \
		exit 1; \
    fi

#-------------------------------------------------------------------------------
include ./makefile-vpi
run: .check-VIRTUAL_ENV $(VPI) check
	$(eval TEST := test)
	$(eval STIMULI := 1)
	@../scripts/sim.sh $(TB) $(TEST) $(.FILE_LIST) $(TIMEOUT) $(DUMPVCD) $(STIMULI) 

#-------------------------------------------------------------------------------
sim: .check-TB .check-TEST .check-MEM_SIZE .check-DSEG_SIZE .check-TIMEOUT .check-DUMPVCD $(VPI) check
	@../scripts/compile_asm_test.sh $(TEST) $(MEM_SIZE) $(DSEG_SIZE)
	@../scripts/sim.sh $(TB) $(TEST) $(.FILE_LIST) $(TIMEOUT) $(DUMPVCD) $(STIMULI) 

#-------------------------------------------------------------------------------
sim-all: .check-TB .check-MEM_SIZE .check-DSEG_SIZE .check-TIMEOUT .check-DUMPVCD $(VPI) check
	@for test in $(basename $(.ASM_TESTS)); do \
		../scripts/compile_asm_test.sh $$test $(MEM_SIZE) $(DSEG_SIZE); \
		../scripts/sim.sh $(TB) $$test $(.FILE_LIST) $(TIMEOUT) $(DUMPVCD) $(STIMULI); \
	done

#-------------------------------------------------------------------------------
sim-c: .check-TB .check-TEST .check-MEM_SIZE .check-DSEG_SIZE .check-TIMEOUT .check-DUMPVCD $(VPI) check
	@../scripts/compile_c_test.sh $(TEST)
	@../scripts/sim.sh $(TB) $(TEST) $(.FILE_LIST) $(TIMEOUT) $(DUMPVCD) $(STIMULI)

#-------------------------------------------------------------------------------
list_asm_tests:
	@echo -e "--------------------------------------------------------------------------"
	@echo -e "Available tests under$(.ASM_TESTS_FOLDER):"
	@echo -e ""
	@for test in $(.ASM_TESTS); do echo -e $$test; done
	@echo -e "--------------------------------------------------------------------------"

#-------------------------------------------------------------------------------
list_c_tests:
	@echo -e "--------------------------------------------------------------------------"
	@echo -e "Available tests under$(.C_TESTS_FOLDER):"
	@echo -e ""
	@for test in $(.C_TESTS); do echo -e $$test; done
	@echo -e "--------------------------------------------------------------------------"

#-------------------------------------------------------------------------------
view: .check-VCD
	@gtkwave $(VCD) &

#-------------------------------------------------------------------------------
clean:
	@echo -e ""
	@rm -rf __pycache__
	rm -rf out
	rm -rf build
	rm -f *.o
	rm -f *.prj
	rm -f *.log
	rm -f *.vpi
	rm -f *.vvp
	rm -f *.vcd
	rm -f *.out
	rm -f *.dot
	@echo -e ""

clean_envs: 
	@python -c 'from stimuli.window import *; clean_envs()'

#-------------------------------------------------------------------------------
distclean: clean
	@make -s -C ../../Software/utils/ clean
	rm -rf $(VENV_NAME)

#-------------------------------------------------------------------------------
# Dependencies
#-------------------------------------------------------------------------------
.Filelist: .check-SYN
	@../scripts/create_filelist.sh $(SYN)
