# Test: load design and run floorplan

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog gcd_nangate45.v
link_design gcd

puts "test: initialize_floorplan"
initialize_floorplan -die_area "0 0 100.13 100.8" \
    -core_area "10.07 11.2 90.25 91" \
    -site FreePDK45_38x28_10R_NP_162NW_34O

puts "test: check floorplan"
set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
set die_area [$block getDieArea]
puts "die area: $die_area"

puts "PASS: tcl_floorplan"
exit 0
