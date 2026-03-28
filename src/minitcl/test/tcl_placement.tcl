# Test: load pre-placed design and verify placement

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_nangate45.def

puts "test: check_placement on pre-placed design"
check_placement

puts "test: improve_placement"
improve_placement

puts "test: check_placement after improve"
check_placement

puts "PASS: tcl_placement"
exit 0
