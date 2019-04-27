#/**************************************************/
#/* Constraint Script for Synopsys                 */
#/*                                                */
#/* dc_shell-t -f constraint_dc.tcl                */
#/*                                                */
#/* OSU FreePDK 45nm                               */
#/**************************************************/

#/* The name of the clock pin. If no clock-pin     */
#/* exists, pick anything                          */
set my_clock_pin clk

#/* Target frequency in MHz for optimization       100*/
set my_clk_freq_MHz 1

#/* Delay of input signals (Clock-to-Q, Package etc.)  0.1*/
set my_input_delay_ns 1

#/* Reserved time for output signals (Holdtime etc.)   0.1*/
set my_output_delay_ns 0.5

set cycle 10  

# set_dont_touch alu 

create_clock -period $cycle [get_ports  clk]
set_dont_touch_network      [get_clocks clk]
set_clock_uncertainty  0.1  [get_clocks clk]
set_clock_latency      0.5  [get_clocks clk]

set_input_delay $my_input_delay_ns -clock clk [remove_from_collection [all_inputs] [get_ports clk]]
set_output_delay $my_output_delay_ns -clock clk [all_outputs]
set_load -pin_load 1  [all_outputs]
set_drive          1  [all_inputs]
set_max_fanout 20 [all_inputs]

