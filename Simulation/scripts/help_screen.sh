#!/bin/bash
#-------------------------------------------------------------------------------
# Copyright (C) 2015 Angel Terrones <angelterrones@gmail.com>
#-------------------------------------------------------------------------------
#
# File Name: help_screen
#
# Author:
#             - Angel Terrones <angelterrones@gmail.com>
#
#-------------------------------------------------------------------------------

###################################################################
# Constants
###################################################################
# text attributes: normal, bold, underline
n='\e[0m';
b='\e[1m';
u='\e[4m';

# bold+green
g='\e[32m';

# bold+red
r='\e[31m';

# brown+blue
B='\e[34m';

###############################################################################
#                                 Help Menu                                   #
###############################################################################
echo -e ""
echo -e "${g}Makefile: ${n} HELP SCREEN"
echo -e ""
echo -e "${b}USAGE:${n}"
echo -e "\tmake ${u}TARGET${n} ${u}VARIABLE${n}"
echo -e ""
echo -e ""
echo -e "${b}TARGETS:${n}"
echo -e "\t${b}help${n}"
echo -e "\t    This help."
echo -e ""
echo -e "\t${b}check${n}"
echo -e "\t    Check Verilog files found in ${u}Hardware/verilog${n} and ${u}Simulation/testbench${n} folders;"
echo -e ""
echo -e "\t${b}list_asm_tests${n}"
echo -e "\t    List all assembler files inside the ${u}Simulation/tests/asm/${n} folder;"
echo -e ""
echo -e "\t${b}list_c_tests${n}"
echo -e "\t    List all C projects inside the ${u}Simulation/tests/c/${n} folder;"
echo -e ""
echo -e "\t${b}run${n}"
echo -e "\t    Instead of Simulating the pre-compiled program, a Stimuli Generator is deployed"
echo -e "\t    to generate pseudo-random instruction, and places all outputs (waveforms, regdump," 
echo -e "\t    logs) in ${b}Simulation/out${n} folder"
echo -e ""
echo -e "\t${b}sim${n}"
echo -e "\t    Simulates a single ASM test, and places all outputs (waveforms, regdump, logs)"
echo -e "\t    in ${b}Simulation/out${n} folder"
echo -e ""
echo -e "\t${b}sim-c${n}"
echo -e "\t    Simulates a single C test, and places all outputs (waveforms, regdump, logs)"
echo -e "\t    in ${b}Simulation/out${n} folder"
echo -e ""
echo -e "\t${b}sim-all${n}"
echo -e "\t    Simulates ${b}all${n} ASM tests, and places all outputs (waveforms, regdump, logs)"
echo -e "\t    in ${b}Simulation/out${n} folder"
echo -e ""
echo -e "\t${b}clean${n}"
echo -e "\t    Clean temporary files inside the ${u}Simulation${n} folder."
echo -e ""
echo -e "\t${b}distclean${n}"
echo -e "\t    Clean ${b}all${n} temporary files."
echo -e ""
echo -e "${b}VARIABLES:${n}"
echo -e "\t${b}SYN${n}=${u}Synthesized gate-levle simulation${n}."
echo -e ""
echo -e "\t${b}TB${n}=${u}Verilog testbench${n}."
echo -e ""
echo -e "\t${b}TEST${n}=${u}ASM test${n}."
echo -e ""
echo -e "\t${b}MEM_SIZE${n}=${u}Memory size${n}."
echo -e ""
echo -e "\t${b}DSEG_SIZE${n}=${u}Size of Data Segment${n}."
echo -e ""
echo -e "\t${b}TIMEOUT${n}=${u}Simulation timeout${n}."
echo -e ""
echo -e "\t${b}DUMPVCD${n}=${u}Generate waveform file${n}."
echo -e ""
echo -e "${b}EXAMPLES:${n}"
echo -e "\tmake"
echo -e "\tmake help"
echo -e "\tmake check"
echo -e "\tmake list_asm_tests"
echo -e "\tmake list_c_tests"
echo -e "\tmake run"
echo -e "\tmake run SYN=1"
echo -e "\tmake sim TEST=<asm-test>"
echo -e "\tmake sim-c TEST=<c-test>"
echo -e "\tmake sim-all"
echo -e "\tmake clean"
echo -e "\tmake distclean"
echo -e ""
echo -e "(END)"
exit 0
