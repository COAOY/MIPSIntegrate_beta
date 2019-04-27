#/* Top-level Module                               */
set my_toplevel antares_core

read_netlist tsmc13.v 
read_netlist antares_defines.v 
read_netlist dft/antares_core_dft.v

set_build -black_box antares_reg_file 
set_build -black_box antares_cpzero

run_build_model $my_toplevel 

# Run DRC
add_clocks 0 clk
read_sdc dft/antares_core_dft.sdc
run_drc dft/antares_core_dft.spf 

set_atpg -full_seq_merge medium
set_atpg -coverage 100
set_faults -model path_delay

add_delay_path syn/antares_core_syn.path
add_faults -all

run_atpg 

write_patterns dft/antares_core_dft.wgl -format WGL
