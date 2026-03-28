# Test: clock tree synthesis

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_nangate45.def
create_clock [get_ports clk] -name core_clock -period 0.4850
set_wire_rc -signal -resistance 1.0 -capacitance 1.0

puts "test: clock_tree_synthesis"
clock_tree_synthesis -root_buf CLKBUF_X3 -buf_list {CLKBUF_X3}

puts "PASS: tcl_cts"
exit 0
