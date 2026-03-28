#!/usr/bin/env bash
# Integration test: run a .tcl script through openroad via source command.
# Usage: openroad_tcl_test.sh <openroad_binary> <tcl_script>
set -e

OPENROAD="$(realpath "$1")"
TCL_SCRIPT="$(realpath "$2")"

# For tests that need design data, cd to the test/ directory in the runfiles
# so relative paths like Nangate45/Nangate45.lef work.
TEST_DATA_DIR="${RUNFILES_DIR:-$TEST_SRCDIR}/_main/test"
if [ -d "$TEST_DATA_DIR" ]; then
    cd "$TEST_DATA_DIR"
fi

echo "Running: source $TCL_SCRIPT"
echo "source $TCL_SCRIPT" | "$OPENROAD" -no_splash
