#/**************************************************/
#/* DFT Compile Script for Synopsys                */
#/*                                                */
#/* dc_shell-t -f compile_dc.tcl                   */
#/*                                                */
#/* OSU FreePDK 45nm                               */
#/**************************************************/

#/* All verilog files, separated by spaces         */
set my_verilog_files [list syn/antares_core_syn.v] 

#/* Top-level Module                               */
set my_toplevel antares_core

read_verilog $my_verilog_files

current_design $my_toplevel

link
    
source constraint_dc.tcl 

set test_default_scan_style multiplexed_flip_flop
set test_default_delay 0
set test_default_bidir_delay 0 
set test_default_strobe 40 
set test_default_period 100

create_test_protocol -infer_asynch -infer_clock

dft_drc

set_scan_configuration -chain_count 8 

preview_dft
insert_dft

dft_drc

check_design

set filename [format "%s%s"  $my_toplevel "_dft.v"]
write -f verilog -hier -output [format "%s%s"  "./dft/" $filename]

set filename [format "%s%s"  $my_toplevel "_dft.spf"]
write_test_protocol -output [format "%s%s"  "./dft/" $filename]

set filename [format "%s%s"  $my_toplevel "_dft.ddc"]
write -f ddc -hier -output [format "%s%s"  "./dft/" $filename]

set filename [format "%s%s"  $my_toplevel "_dft.sdf"]
write_sdf -version 2.1 [format "%s%s"  "./dft/" $filename]

set filename [format "%s%s"  $my_toplevel "_dft.spef"]
write_parasitics -output [format "%s%s"  "./dft/" $filename]

set filename [format "%s%s"  $my_toplevel "_dft.scan_path"]
redirect [format "%s%s"  "./rpt/" $filename] { report_constraint -all_violators -verbose }

set filename [format "%s%s"  $my_toplevel "_dft.cell"]
redirect [format "%s%s"  "./rpt/" $filename] { report_constraint -all_violators -verbose }

redirect [format "%s%s"  "./rpt/" violation.rpt] { report_constraint -all_violators -verbose }
redirect [format "%s%s"  [format "%s%s"  "./rpt/" $my_toplevel] ".area"] { report_area }
redirect [format "%s%s"  "./rpt/" timing.rpt] { report_timing }
redirect [format "%s%s"  "./rpt/" cell.rpt] { report_cell }
redirect [format "%s%s"  "./rpt/" power.rpt] { report_power }
