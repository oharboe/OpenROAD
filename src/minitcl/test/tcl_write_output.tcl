# Test: load design, modify, write outputs

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_nangate45.def

puts "test: improve_placement"
improve_placement

puts "test: write_def"
set def_out "/tmp/minitcl_test_gcd_out.def"
write_def $def_out
if {![file exists $def_out]} { error "DEF not written" }
puts "wrote DEF: [file size $def_out] bytes"

puts "test: write_verilog"
set v_out "/tmp/minitcl_test_gcd_out.v"
write_verilog $v_out
if {![file exists $v_out]} { error "Verilog not written" }
puts "wrote Verilog: [file size $v_out] bytes"

puts "PASS: tcl_write_output"
exit 0
