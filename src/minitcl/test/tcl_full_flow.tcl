# Test: complete OpenROAD flow on Nangate45 GCD
# LEF → Liberty → DEF → CTS → GRT → DRT → write_def

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_nangate45.def

puts "test: create_clock + set_wire_rc"
create_clock [get_ports clk] -name core_clock -period 0.4850
set_wire_rc -signal -resistance 1.0 -capacitance 1.0

puts "test: improve_placement"
improve_placement

puts "test: clock_tree_synthesis"
clock_tree_synthesis -root_buf CLKBUF_X3 -buf_list {CLKBUF_X3}

puts "test: set_routing_layers + global_route"
set_routing_layers -signal metal2-metal10
global_route

puts "test: detailed_route"
detailed_route -save_guide_updates

puts "test: write_def"
write_def /tmp/minitcl_full_flow.def

puts "PASS: tcl_full_flow"
exit 0
