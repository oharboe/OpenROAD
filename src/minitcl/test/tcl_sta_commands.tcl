# Test: STA Tcl commands are available after init
# Verifies that the STA init scripts loaded correctly

puts "test: sta utility procs exist"
foreach cmd {
    parse_key_args check_argc_eq0 check_argc_eq1
    is_keyword_arg define_cmd_args proc_redirect
} {
    if {[info commands sta::$cmd] eq ""} {
        error "sta::$cmd not found"
    }
}

puts "test: sta flow commands exist"
foreach cmd {
    read_liberty read_verilog link_design
    create_clock report_checks
} {
    if {[info commands $cmd] eq ""} {
        error "$cmd not found (namespace import may have failed)"
    }
}

puts "test: openroad commands exist"
foreach cmd {
    read_lef read_def write_def
    initialize_floorplan global_placement detailed_placement
    clock_tree_synthesis global_route detailed_route
} {
    if {[info commands $cmd] eq ""} {
        error "$cmd not found"
    }
}

puts "test: is_keyword_arg correctness"
if {[sta::is_keyword_arg "-tech"] != 1} { error "is_keyword_arg -tech should be 1" }
if {[sta::is_keyword_arg "/dev/null"] != 0} { error "is_keyword_arg /dev/null should be 0" }
if {[sta::is_keyword_arg "foo.lef"] != 0} { error "is_keyword_arg foo.lef should be 0" }

puts "PASS: tcl_sta_commands"
exit 0
