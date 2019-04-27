#/**************************************************/
#/* Compile Script for Synopsys                    */
#/*                                                */
#/* pt_shell-t -f compile_dc.tcl                   */
#/*                                                */
#/* OSU FreePDK 45nm                               */
#/**************************************************/

#/* All verilog files, separated by spaces         */
set my_verilog_files [list ./syn/antares_core_syn.v ]

#/* Top-level Module                               */
set my_toplevel antares_core

read_verilog $my_verilog_files

lappend link_path antares_core_syn.v 
# set link_create_black_boxes false

current_design $my_toplevel
link_design $my_toplevel 

set filename [format "%s%s"  $my_toplevel "_syn.spef"]
read_parasitics [format "%s%s"  "./syn/" $filename]

set filename [format "%s%s"  $my_toplevel "_syn.sdc"]
source [format "%s%s"  "./syn/" $filename]

check_timing 
report_timing 
report_bottleneck

source pt2tmax.tcl
set filename [format "%s%s"  $my_toplevel "_syn.path"]
write_delay_paths -max_paths 10 [format "%s%s"  "./syn/" $filename]
