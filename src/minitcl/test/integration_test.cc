// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Integration tests simulating real OpenROAD usage

#include "tcl.h"
#include <cstring>
#include <gtest/gtest.h>

// Test that empty commands don't produce errors
TEST(IntegrationTest, EmptyLinesInScript) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "\n\n\nset x 1\n\n\n"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "1");
    Tcl_DeleteInterp(interp);
}

// Test that trailing semicolons and newlines are fine
TEST(IntegrationTest, TrailingSemicolonsAndNewlines) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "set x 1;\n;\n;\n"), TCL_OK);
    Tcl_DeleteInterp(interp);
}

// Test evalTclInit pattern: eval a large multi-command script via Tcl_EvalObjEx
TEST(IntegrationTest, EvalObjExMultiCommand) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    const char* script =
        "proc helper {x} { return $x }\n"
        "proc main {} { return [helper 42] }\n";
    Tcl_Obj* obj = Tcl_NewStringObj(script, -1);
    Tcl_IncrRefCount(obj);
    ASSERT_EQ(Tcl_EvalObjEx(interp, obj, 0), TCL_OK);
    Tcl_DecrRefCount(obj);

    ASSERT_EQ(Tcl_Eval(interp, "main"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");
    Tcl_DeleteInterp(interp);
}

// Test the namespace eval + proc pattern from STA init scripts
TEST(IntegrationTest, StaInitPattern) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    const char* script =
        "namespace eval sta {\n"
        "  variable cmd_args\n"
        "\n"
        "  proc define_cmd_args { cmd arglist } {\n"
        "    variable cmd_args\n"
        "    set cmd_args($cmd) $arglist\n"
        "  }\n"
        "}\n"
        "\n"
        "namespace import sta::*\n";

    ASSERT_EQ(Tcl_Eval(interp, script), TCL_OK);
    ASSERT_EQ(Tcl_Eval(interp, "define_cmd_args test_cmd {arg1 arg2}"), TCL_OK);
    Tcl_DeleteInterp(interp);
}

// Test the OpenRoad init sequence: define_sta_cmds + namespace import
TEST(IntegrationTest, OpenRoadInitSequence) {
    Tcl_Interp* interp = Tcl_CreateInterp();

    // Simulate what OpenRoad.cc does
    const char* staInit =
        "namespace eval sta {\n"
        "  proc define_sta_cmds {} {\n"
        "    # placeholder\n"
        "  }\n"
        "  proc init_sta_cmds {} {\n"
        "    # placeholder\n"
        "  }\n"
        "}\n";

    ASSERT_EQ(Tcl_Eval(interp, staInit), TCL_OK);
    ASSERT_EQ(Tcl_Eval(interp, "sta::define_sta_cmds"), TCL_OK);
    ASSERT_EQ(Tcl_Eval(interp, "namespace import sta::*"), TCL_OK);

    Tcl_DeleteInterp(interp);
}

// Test that $errorInfo variable works
TEST(IntegrationTest, ErrorInfo) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "error oops");
    // $errorInfo should be queryable
    ASSERT_EQ(Tcl_Eval(interp, "set errorInfo"), TCL_OK);
    Tcl_DeleteInterp(interp);
}

// Test Tcl_Eval of $errorInfo (used by evalTclInit error path)
TEST(IntegrationTest, EvalErrorInfoVariable) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // This is what evalTclInit does on error:
    // Tcl_Eval(interp, "$errorInfo");
    // This should substitute $errorInfo and eval the result
    Tcl_SetVar(interp, "errorInfo", "some error trace", TCL_GLOBAL_ONLY);
    int code = Tcl_Eval(interp, "$errorInfo");
    // The result of evaling "$errorInfo" is evaling "some error trace"
    // which would be "invalid command name 'some'" - that's expected
    // The point is it shouldn't crash
    (void)code;
    Tcl_DeleteInterp(interp);
}

// Test script with comments between commands
TEST(IntegrationTest, CommentsInMultiLineScript) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    const char* script =
        "# Setup\n"
        "set x 1\n"
        "# Middle comment\n"
        "set y 2\n"
        "# End comment\n";
    ASSERT_EQ(Tcl_Eval(interp, script), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "1");
    EXPECT_STREQ(Tcl_GetVar(interp, "y", 0), "2");
    Tcl_DeleteInterp(interp);
}

// Test realistic flow script patterns
TEST(IntegrationTest, FlowScriptPattern) {
    Tcl_Interp* interp = Tcl_CreateInterp();

    // Set up env-like variables
    Tcl_SetVar(interp, "::env(SCRIPTS_DIR)", "/scripts", TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "::env(RESULTS_DIR)", "/results", TCL_GLOBAL_ONLY);

    const char* script =
        "proc env_var_exists_and_non_empty {var_name} {\n"
        "  if {[info exists ::env($var_name)]} {\n"
        "    if {![string equal $::env($var_name) {}]} {\n"
        "      return 1\n"
        "    }\n"
        "  }\n"
        "  return 0\n"
        "}\n";

    ASSERT_EQ(Tcl_Eval(interp, script), TCL_OK);
    ASSERT_EQ(Tcl_Eval(interp, "env_var_exists_and_non_empty SCRIPTS_DIR"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "1");
    ASSERT_EQ(Tcl_Eval(interp, "env_var_exists_and_non_empty NONEXISTENT"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "0");

    Tcl_DeleteInterp(interp);
}

// Test ORFS patterns: quotes inside brackets in double-quoted strings,
// array variables in expr, and expr short-circuit evaluation.
// These patterns appear in ORFS util.tcl log_cmd and flow control.
TEST(IntegrationTest, OrfsLogCmdPattern) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // The log_cmd pattern: "$cmd[join [lmap arg $args { ... "\"$arg\"" ... }]]"
    ASSERT_EQ(Tcl_Eval(interp,
        "proc log_fmt { cmd args } {\n"
        "  set log \"$cmd[join [lmap arg $args { format \" %s\" "
        "[expr { [string match {* *} $arg] ? \"\\\"$arg\\\"\" : \"$arg\" }]"
        " }] \"\"]\"\n"
        "  return $log\n"
        "}\n"
        "log_fmt read_liberty /tmp/test.lib"
    ), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "read_liberty /tmp/test.lib");

    // Test with spaces in arg (should get quoted)
    ASSERT_EQ(Tcl_Eval(interp, "log_fmt source {path with spaces}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "source \"path with spaces\"");
    Tcl_DeleteInterp(interp);
}

TEST(IntegrationTest, OrfsEnvArrayInExpr) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // ORFS pattern: if { $::env(KEEP_VARS) } { return }
    Tcl_SetVar(interp, "env(KEEP_VARS)", "0", TCL_GLOBAL_ONLY);
    ASSERT_EQ(Tcl_Eval(interp,
        "proc check_keep {} {\n"
        "  if { $::env(KEEP_VARS) } { return 1 }\n"
        "  return 0\n"
        "}\n"
        "check_keep"
    ), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "0");
    Tcl_DeleteInterp(interp);
}

TEST(IntegrationTest, OrfsNullShortCircuit) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // ORFS pattern: [$db getChip] != "NULL" && [[$db getChip] getBlock] != "NULL"
    // When getChip returns "NULL", the second branch must NOT be evaluated
    ASSERT_EQ(Tcl_Eval(interp,
        "proc getChip {} { return NULL }\n"
        "proc test_null {} {\n"
        "  if { [getChip] != \"NULL\" && [[getChip] getBlock] != \"NULL\" } {\n"
        "    return loaded\n"
        "  }\n"
        "  return empty\n"
        "}\n"
        "test_null"
    ), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "empty");
    Tcl_DeleteInterp(interp);
}

// Test {*} argument expansion (used in ORFS scripts)
TEST(IntegrationTest, ArgumentExpansion) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // {*} is Tcl 8.5+ argument expansion
    // For now, test that basic list expansion works via eval
    Tcl_Eval(interp, "set args {a b c}");
    // We may not support {*} yet, but test what we can
    ASSERT_EQ(Tcl_Eval(interp, "list {*}$args"), TCL_OK);
    // If {*} isn't supported, this might fail - that's OK for now
    Tcl_DeleteInterp(interp);
}
