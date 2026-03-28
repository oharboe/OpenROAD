# Test: global route + detailed route + write_def

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_nangate45.def

puts "test: set_routing_layers"
set_routing_layers -signal metal2-metal10

puts "test: global_route"
global_route

puts "test: detailed_route"
detailed_route -save_guide_updates

puts "PASS: tcl_route"
exit 0
