#!/bin/bash
#-------------------------------------------------------------------------------
# Copyright (C) 2014 Angel Terrones <angelterrones@gmail.com>
#-------------------------------------------------------------------------------
#
# File Name: create_filelist
#
# Author:
#             - Angel Terrones <angelterrones@gmail.com>
#
#-------------------------------------------------------------------------------

# Set the folders

CURRENT_FOLDER="$(pwd)"
PROJECT_ROOT="${CURRENT_FOLDER%/Simulation*}"

RTL_FOLDER=$PROJECT_ROOT/Hardware/verilog
SYN_FOLDER=$PROJECT_ROOT/Hardware/syn
RUN_FOLDER=$PROJECT_ROOT/Simulation/run
TESTBENCH_FOLDER=$PROJECT_ROOT/Simulation/bench

TECH_FILE=$SYN_FOLDER/tsmc13.v 
REG_FILE=$RTL_FOLDER/antares_reg_file.v
DEF_FILE=$RTL_FOLDER/antares_defines.v
CP0_FILE=$RTL_FOLDER/antares_cpzero.v

# Parameter Check
EXPECTED_ARGS=1
if [ $# -ne $EXPECTED_ARGS ]; then
    echo
    echo -e "ERROR      : wrong number of arguments"
    echo -e "USAGE      : create_filelist <Synthesized>" 
    echo -e "Example    : create_filelist 1"
    echo
    exit 1
fi

# Set Filelist variable
# Create the file inside $RUN_FOLDER
mkdir -p $RUN_FOLDER/out
FILELIST_ICARUS=$RUN_FOLDER/out/filelist.prj

# remove old files
rm -f $RUN_FOLDER/out/*.prj

#create the new filelist of rtl
touch $FILELIST_ICARUS

if [ $1 -eq 1 ]; then 
    find $SYN_FOLDER -name "*.v" >> $FILELIST_ICARUS
    echo $REG_FILE >> $FILELIST_ICARUS
    echo $DEF_FILE >> $FILELIST_ICARUS
    echo $CP0_FILE >> $FILELIST_ICARUS
else 
    find $RTL_FOLDER -name "*.v" >> $FILELIST_ICARUS
fi 
find $TESTBENCH_FOLDER -name "*.v" >> $FILELIST_ICARUS
find $CURRENT_FOLDER -name "*.sft" >> $FILELIST_ICARUS

#-------------------------------------------------------------------------------
# Xilinx libraries.
# echo +libdir+/opt/Xilinx/14.7/ISE_DS/ISE/verilog/src/unisims >> $FILELIST_ICARUS
#-------------------------------------------------------------------------------

if [ $1 -eq 1 ]; then 
    for folder in $(find $SYN_FOLDER -type d)
    do
        echo "+incdir+"$folder >> $FILELIST_ICARUS
    done
fi 

for folder in $(find $RTL_FOLDER -type d)
do
    echo "+incdir+"$folder >> $FILELIST_ICARUS
done
