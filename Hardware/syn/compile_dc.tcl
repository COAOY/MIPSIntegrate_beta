#/**************************************************/
#/* Compile Script for Synopsys                    */
#/*                                                */
#/* dc_shell-t -f compile_dc.tcl                   */
#/*                                                */
#/* OSU FreePDK 45nm                               */
#/**************************************************/

#/* Define working directory 			   */ 
set WORK ./syn
define_design_lib SYN -path $WORK

#/* All verilog files, separated by spaces         */
set my_verilog_files [list antares_add.v antares_alu.v antares_branch_unit.v antares_cloz.v antares_control_unit.v antares_core.v antares_defines.v antares_divider.v antares_exmem_register.v antares_hazard_unit.v antares_idex_register.v antares_ifid_register.v antares_load_store_unit.v antares_memwb_register.v antares_multiplier.v antares_mux_2_1.v antares_mux_4_1.v antares_pc_register.v antares_shifter.v] 

#/* Top-level Module                               */
set my_toplevel antares_core

analyze -f verilog $my_verilog_files
elaborate $my_toplevel

current_design $my_toplevel

link
    
source constraint_dc.tcl 
set_max_area 0 
set_max_fanout 8 $my_toplevel

compile_ultra -no_autoungroup

check_design
report_constraint -all_violators

set filename [format "%s%s"  $my_toplevel "_syn.v"]
write -f verilog -hier -output [format "%s%s"  "./syn/" $filename]

set filename [format "%s%s"  $my_toplevel "_syn.sdc"]
write_sdc [format "%s%s"  "./syn/" $filename]

set filename [format "%s%s"  $my_toplevel "_syn.ddc"]
write -f ddc -hier -output [format "%s%s"  "./syn/" $filename]

set filename [format "%s%s"  $my_toplevel "_syn.sdf"]
write_sdf -version 2.1 [format "%s%s"  "./syn/" $filename]

set filename [format "%s%s"  $my_toplevel "_syn.spef"]
write_parasitics -output [format "%s%s"  "./syn/" $filename]

redirect [format "%s%s"  "./rpt/" violation.rpt] { report_constraint -all_violators -verbose }
redirect [format "%s%s"  [format "%s%s"  "./rpt/" $my_toplevel] ".area"] { report_area }
redirect [format "%s%s"  "./rpt/" timing.rpt] { report_timing }
redirect [format "%s%s"  "./rpt/" cell.rpt] { report_cell }
redirect [format "%s%s"  "./rpt/" power.rpt] { report_power }
