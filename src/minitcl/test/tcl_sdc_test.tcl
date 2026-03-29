# Test: read_sdc with set_input_delay/set_output_delay
# This tests the ORFS SDC pattern that uses lsearch, expr in SDC context

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_nangate45.def

# Write a SDC file similar to ORFS constraint.sdc (using env vars)
set ::env(ABC_CLOCK_PERIOD_IN_PS) 485
set fd [open /tmp/test_orfs.sdc w]
puts $fd {current_design gcd}
puts $fd {set clk_name core_clock}
puts $fd {set clk_port_name clk}
puts $fd {set clk_period $::env(ABC_CLOCK_PERIOD_IN_PS)}
puts $fd {set clk_io_pct 0.2}
puts $fd {set clk_port [get_ports $clk_port_name]}
puts $fd {create_clock -name $clk_name -period $clk_period $clk_port}
puts $fd {set non_clock_inputs [lsearch -inline -all -not -exact [all_inputs] $clk_port]}
puts $fd {set_input_delay [expr $clk_period * $clk_io_pct] -clock $clk_name $non_clock_inputs}
puts $fd {set_output_delay [expr $clk_period * $clk_io_pct] -clock $clk_name [all_outputs]}
close $fd

puts "test: read_sdc with env var and set_input_delay/set_output_delay"
read_sdc /tmp/test_orfs.sdc

puts "PASS: tcl_sdc_test"
exit 0
