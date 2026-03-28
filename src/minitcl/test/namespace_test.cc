// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 9 - Namespace, upvar, uplevel tests

#include "tcl.h"
#include <cstring>
#include <gtest/gtest.h>

// ============================================================
// namespace eval
// ============================================================
TEST(NamespaceTest, EvalDefinesProc) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i,
        "namespace eval myns { proc greet {} { return hello } }"), TCL_OK);
    // The proc should be callable (registered in myns namespace or globally)
    ASSERT_EQ(Tcl_Eval(i, "greet"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hello");
    Tcl_DeleteInterp(i);
}

TEST(NamespaceTest, EvalSetsVar) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i,
        "namespace eval myns { set x 42 }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "42");
    Tcl_DeleteInterp(i);
}

TEST(NamespaceTest, Current) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "namespace current"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "::");
    Tcl_DeleteInterp(i);
}

TEST(NamespaceTest, CurrentInsideEval) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i,
        "namespace eval foo { namespace current }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "::foo");
    Tcl_DeleteInterp(i);
}

TEST(NamespaceTest, ExportNoError) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "namespace export myproc"), TCL_OK);
    Tcl_DeleteInterp(i);
}

// ============================================================
// upvar
// ============================================================
TEST(UpvarTest, UpvarReadsCaller) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc reader {varName} { upvar 1 $varName local; return $local }");
    Tcl_Eval(i, "set x 42");
    ASSERT_EQ(Tcl_Eval(i, "reader x"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "42");
    Tcl_DeleteInterp(i);
}

TEST(UpvarTest, UpvarWritesCaller) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc writer {varName val} { upvar 1 $varName local; set local $val }");
    Tcl_Eval(i, "set x 0");
    Tcl_Eval(i, "writer x 99");
    ASSERT_EQ(Tcl_Eval(i, "set x"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "99");
    Tcl_DeleteInterp(i);
}

TEST(UpvarTest, UpvarGlobal) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set g 100");
    Tcl_Eval(i, "proc f {} { upvar #0 g local; return $local }");
    ASSERT_EQ(Tcl_Eval(i, "f"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "100");
    Tcl_DeleteInterp(i);
}

// ============================================================
// uplevel
// ============================================================
TEST(UplevelTest, UplevelExecutesInCaller) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc f {} { set local_val 77; uplevel 1 {set captured $local_val} }");
    // This should fail because local_val isn't in caller scope
    // Actually uplevel 1 runs in caller's scope. local_val is set in f's frame.
    // When we uplevel, we pop back to caller where local_val doesn't exist.
    // Let's test a simpler case:
    Tcl_DeleteInterp(i);

    i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc f {} { uplevel 1 {set result_from_uplevel 42} }");
    Tcl_Eval(i, "f");
    ASSERT_EQ(Tcl_Eval(i, "set result_from_uplevel"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "42");
    Tcl_DeleteInterp(i);
}

TEST(UplevelTest, UplevelGlobal) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc f {} { uplevel #0 {set global_result hello} }");
    Tcl_Eval(i, "f");
    ASSERT_EQ(Tcl_Eval(i, "set global_result"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hello");
    Tcl_DeleteInterp(i);
}

// ============================================================
// variable
// ============================================================
TEST(VariableTest, VariableInProc) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set myvar 55");
    Tcl_Eval(i, "proc f {} { variable myvar; return $myvar }");
    ASSERT_EQ(Tcl_Eval(i, "f"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "55");
    Tcl_DeleteInterp(i);
}

TEST(VariableTest, VariableWithInit) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "variable x 123"), TCL_OK);
    ASSERT_EQ(Tcl_Eval(i, "set x"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "123");
    Tcl_DeleteInterp(i);
}

// ============================================================
// rename
// ============================================================
TEST(RenameTest, RenameCommand) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc hello {} { return hi }");
    ASSERT_EQ(Tcl_Eval(i, "rename hello greet"), TCL_OK);
    ASSERT_EQ(Tcl_Eval(i, "greet"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hi");
    EXPECT_EQ(Tcl_Eval(i, "hello"), TCL_ERROR);  // old name gone
    Tcl_DeleteInterp(i);
}

TEST(RenameTest, DeleteCommand) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc hello {} { return hi }");
    ASSERT_EQ(Tcl_Eval(i, "rename hello {}"), TCL_OK);
    EXPECT_EQ(Tcl_Eval(i, "hello"), TCL_ERROR);
    Tcl_DeleteInterp(i);
}

// ============================================================
// namespace eval with proc (STA pattern)
// ============================================================
TEST(NamespaceTest, StaPattern) {
    Tcl_Interp* i = Tcl_CreateInterp();
    // This mimics the STA pattern: namespace eval sta { proc ... }
    ASSERT_EQ(Tcl_Eval(i, R"(
        namespace eval sta {
            proc check_argc_eq0 {cmd argc} {
                if {$argc != 0} {
                    return "wrong # args"
                }
                return ok
            }
        }
    )"), TCL_OK);

    ASSERT_EQ(Tcl_Eval(i, "check_argc_eq0 test 0"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "ok");
    Tcl_DeleteInterp(i);
}
