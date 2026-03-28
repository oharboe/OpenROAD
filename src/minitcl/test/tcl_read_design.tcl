# Test: load a complete Nangate45 GCD design (LEF + Liberty + DEF)

puts "test: read_lef"
read_lef Nangate45/Nangate45.lef

puts "test: read_liberty"
read_liberty Nangate45/Nangate45_typ.lib

puts "test: read_def"
read_def gcd_nangate45.def

puts "test: design loaded - check basic queries"
set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
set design_name [$block getName]
puts "design name: $design_name"
if {$design_name ne "gcd"} { error "expected design name 'gcd', got '$design_name'" }

puts "PASS: tcl_read_design"
exit 0
