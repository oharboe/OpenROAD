# Smoke test: basic Tcl functionality inside openroad
# Tests: puts, set, expr, proc, if, namespace, info commands

puts "test: basic set/expr"
set x 42
set y [expr {$x + 8}]
if {$y != 50} { error "expr failed: $y" }

puts "test: proc definition and call"
proc add {a b} { return [expr {$a + $b}] }
if {[add 3 4] != 7} { error "proc failed" }

puts "test: namespace eval"
namespace eval myns {
    proc greet {} { return hello }
}
if {[myns::greet] != "hello"} { error "namespace proc failed" }

puts "test: string commands"
if {[string length "abc"] != 3} { error "string length failed" }
if {[string index "abc" 1] != "b"} { error "string index failed" }

puts "test: list commands"
set lst {a b c d}
if {[llength $lst] != 4} { error "llength failed" }
if {[lindex $lst 2] != "c"} { error "lindex failed" }

puts "test: info commands exist"
if {[info commands puts] eq ""} { error "puts not found" }
if {[info commands set] eq ""} { error "set not found" }
if {[info commands expr] eq ""} { error "expr not found" }

puts "PASS: tcl_smoke"
exit 0
