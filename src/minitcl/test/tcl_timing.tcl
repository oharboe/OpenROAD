# Test: STA timing analysis

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_nangate45.def
create_clock [get_ports clk] -name core_clock -period 0.4850
set_wire_rc -signal -resistance 1.0 -capacitance 1.0

puts "test: report_checks"
report_checks

puts "test: report_tns"
report_tns

puts "test: report_wns"
report_wns

puts "test: report_design_area"
report_design_area

puts "PASS: tcl_timing"
exit 0
