# insert_buffer across hierarchy on a bus-bit net: the punched hierarchical
# ports/nets derive their names from the flat net base name "mid[5]"; the
# brackets must be flattened so the written netlist does not reference a
# select of a bus that does not exist in the module.
source "helpers.tcl"

set test_name insert_buffer_hier_bus

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog insert_buffer_hier_bus.v
link_design -hier top
initialize_floorplan -die_area {0 0 40 40} -core_area {2 2 38 38} \
  -site FreePDK45_38x28_10R_NP_162NW_34O

set net [get_nets mid[5]]
set load [get_pins {u_load1/buf1/A u_load2/buf1/A}]
set buf1 [insert_buffer \
  -net $net \
  -load_pins $load \
  -buffer_cell BUF_X1 \
  -buffer_name b_hier_bus \
  -net_name n_hier_bus]
puts "Inserted: [get_name $buf1]"

set out_verilog [make_result_file "${test_name}.v"]
write_verilog $out_verilog
diff_files "${test_name}.vok" $out_verilog
