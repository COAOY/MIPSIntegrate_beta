#!/bin/bash
#-------------------------------------------------------------------------------
# Copyright (C) 2018 C.Y. Chen <chingyuanchen@g.ntu.edu.tw>
#-------------------------------------------------------------------------------
#
# File Name: sim.sh
#
# Author:
#             - C.Y. Chen <chingyuanchen@g.ntu.edu.tw>
# Description:
#       Compile & run project Project
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Folders
#-------------------------------------------------------------------------------
CURRENT_FOLDER="$(pwd)"
PROJECT_ROOT="${CURRENT_FOLDER%/Simulation*}"
OUT_FOLDER=$PROJECT_ROOT/Simulation/run/out
ASM_FOLDER=$PROJECT_ROOT/Simulation/tests/asm
TESTBENCH_FOLDER=$PROJECT_ROOT/Simulation/bench
SCRIPT_FOLDER=$PROJECT_ROOT/Simulation/scripts

#-------------------------------------------------------------------------------
# Parameter Check
#-------------------------------------------------------------------------------
EXPECTED_ARGS=6
if [ $# -ne $EXPECTED_ARGS ]; then
    echo
    echo -e "ERROR      : wrong number of arguments"
    echo -e "USAGE      : sim <Testbench file> <test name> <include list> <timeout (n° cycles)> <dump VCD> <stimuli>"
    echo -e "Example    : sim tb_core addiu filelist.prj 10000 1 1"
    echo -e "Example    : sim tb_core addiu filelist.prj 60000 0 1"
    echo
    exit 1
fi

#-------------------------------------------------------------------------------
# Check if filelist exist
#-------------------------------------------------------------------------------
tb=$TESTBENCH_FOLDER/verilog/$1.v;

if [ ! -e ${tb} ]; then
    echo
    echo -e "ERROR:\tTestbench file doesn't exist: $(readlink -f ${tb})"
    echo
    exit 1
fi

if [ ! -f ${tb} ]; then
    echo
    echo -e "ERROR:\tTestbench argument is not a file: $(readlink -f ${tb})"
    echo
    exit 1
fi

#-------------------------------------------------------------------------------
# Compile testbench
#-------------------------------------------------------------------------------
mkdir -p $OUT_FOLDER
echo -e "--------------------------------------------------------------------------"
echo -e "INFO:\tCompiling Testbench: $(readlink -f ${tb})"

# parameters
if [ $5 -eq 0 ]; then
    NODUMP="-D NODUMP"
else
    NODUMP=""
fi

if [ $6 -eq 1 ]; then
    STIMULI="-D STIMULI"
else
    STIMULI=""
fi

echo -e "INFO:\tVPI Source compilation: DONE."
echo -e "--------------------------------------------------------------------------"

# compile testbench, use include list, and generate vvp file
if !(iverilog -c$3 -s $1 -o $OUT_FOLDER/$1.vvp -DTIMEOUT=$4 ${NODUMP} ${STIMULI} -DTEST=\"$2\") then
    echo -e "ERROR:\tCompile error: TB = $(readlink -f ${tb})"
    echo -e "--------------------------------------------------------------------------"
    exit 1
fi
echo -e "INFO:\tTestbench compilation: DONE."
echo -e "--------------------------------------------------------------------------"

#-------------------------------------------------------------------------------
# Run testbench
#-------------------------------------------------------------------------------
echo -e "--------------------------------------------------------------------------"
echo -e "INFO:\tStart Verilog simulation."
echo -e "--------------------------------------------------------------------------"
#cp $2 $3
cd $OUT_FOLDER
if !(vvp -n -M.. -mstimuli $1.vvp) then
    echo
    echo -e "ERROR -- Simulation error. Check input files: TB=$1, MIF=$2"
    echo
    exit 1
fi

#-------------------------------------------------------------------------------
# Move log files
#-------------------------------------------------------------------------------
mkdir -p ${OUT_FOLDER}/log
mv ${OUT_FOLDER}/register.log ${OUT_FOLDER}/log/$2-register.log
mv ${OUT_FOLDER}/memory.log ${OUT_FOLDER}/log/$2-memory.log
mv ${OUT_FOLDER}/trace.log ${OUT_FOLDER}/log/$2-trace.log

#-------------------------------------------------------------------------------
# Check result
#-------------------------------------------------------------------------------
if [ $6 -eq 0 ]; then
    ${SCRIPT_FOLDER}/check_result.py ${OUT_FOLDER}/log/$2-register.log ${ASM_FOLDER}/$2.reg
fi

#-------------------------------------------------------------------------------
# Show last message
#-------------------------------------------------------------------------------
echo -e "--------------------------------------------------------------------------"
echo -e "INFO:\tVerilog simulation: DONE."
echo -e "--------------------------------------------------------------------------"
