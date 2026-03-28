// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 4 - proc, return, error, global, variable scoping tests

#include "tcl.h"

#include <cstring>
#include <gtest/gtest.h>

// ============================================================
// proc definition and call
// ============================================================

TEST(ProcTest, SimpleProc) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "proc greet {} { set x hello }"), TCL_OK);
    ASSERT_EQ(Tcl_Eval(interp, "greet"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, ProcWithOneArg) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc double {x} { set result \"$x$x\" }");
    ASSERT_EQ(Tcl_Eval(interp, "double hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hellohello");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, ProcWithTwoArgs) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc add {a b} { set result \"$a $b\" }");
    ASSERT_EQ(Tcl_Eval(interp, "add hello world"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello world");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, ProcWrongArgCount) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {x} { set x }");
    EXPECT_EQ(Tcl_Eval(interp, "f"), TCL_ERROR);
    const char* err = Tcl_GetStringResult(interp);
    EXPECT_NE(strstr(err, "wrong # args"), nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, ProcTooManyArgs) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {x} { set x }");
    EXPECT_EQ(Tcl_Eval(interp, "f 1 2"), TCL_ERROR);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Default arguments
// ============================================================

TEST(ProcTest, DefaultArgUsed) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc greet {{name world}} { set result \"hello $name\" }");
    ASSERT_EQ(Tcl_Eval(interp, "greet"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello world");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, DefaultArgOverridden) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc greet {{name world}} { set result \"hello $name\" }");
    ASSERT_EQ(Tcl_Eval(interp, "greet tcl"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello tcl");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// args (variadic)
// ============================================================

TEST(ProcTest, ArgsEmpty) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {args} { set args }");
    ASSERT_EQ(Tcl_Eval(interp, "f"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, ArgsMultiple) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {args} { set args }");
    ASSERT_EQ(Tcl_Eval(interp, "f a b c"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "a b c");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, ArgsWithFixedParams) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {x args} { set result \"$x:$args\" }");
    ASSERT_EQ(Tcl_Eval(interp, "f first a b"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "first:a b");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// return
// ============================================================

TEST(ProcTest, ReturnValue) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {} { return 42 }");
    ASSERT_EQ(Tcl_Eval(interp, "f"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, ReturnStopsExecution) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {} { return early; set x never }");
    ASSERT_EQ(Tcl_Eval(interp, "f"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "early");
    EXPECT_EQ(Tcl_GetVar(interp, "x", 0), nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, ReturnNoValue) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {} { return }");
    ASSERT_EQ(Tcl_Eval(interp, "f"), TCL_OK);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// error
// ============================================================

TEST(ProcTest, ErrorInProc) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {} { error \"something went wrong\" }");
    EXPECT_EQ(Tcl_Eval(interp, "f"), TCL_ERROR);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "something went wrong");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, ErrorStopsExecution) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {} { error oops; set x never }");
    EXPECT_EQ(Tcl_Eval(interp, "f"), TCL_ERROR);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Variable scoping
// ============================================================

TEST(ProcTest, LocalVariablesNotVisibleOutside) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {} { set local_var 42 }");
    Tcl_Eval(interp, "f");
    EXPECT_EQ(Tcl_GetVar(interp, "local_var", 0), nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, GlobalNotVisibleInProc) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set g 100");
    Tcl_Eval(interp, "proc f {} { set g }");
    EXPECT_EQ(Tcl_Eval(interp, "f"), TCL_ERROR);
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, GlobalCommandMakesVarVisible) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set g 100");
    Tcl_Eval(interp, "proc f {} { global g; set g }");
    ASSERT_EQ(Tcl_Eval(interp, "f"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "100");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, GlobalCommandWritesBack) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set g 1");
    Tcl_Eval(interp, "proc f {} { global g; set g 2 }");
    Tcl_Eval(interp, "f");
    ASSERT_EQ(Tcl_Eval(interp, "set g"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "2");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, DoubleColonAccessesGlobal) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set g 99");
    Tcl_Eval(interp, "proc f {} { set ::g }");
    ASSERT_EQ(Tcl_Eval(interp, "f"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "99");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, DoubleColonWriteGlobal) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc f {} { set ::g 77 }");
    Tcl_Eval(interp, "f");
    ASSERT_EQ(Tcl_Eval(interp, "set g"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "77");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Nested proc calls
// ============================================================

TEST(ProcTest, NestedProcCalls) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc inner {} { return 42 }");
    Tcl_Eval(interp, "proc outer {} { return [inner] }");
    ASSERT_EQ(Tcl_Eval(interp, "outer"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");
    Tcl_DeleteInterp(interp);
}

TEST(ProcTest, NestedScopesIsolated) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc inner {} { set x inner_x; return $x }");
    Tcl_Eval(interp, "proc outer {} { set x outer_x; inner; return $x }");
    ASSERT_EQ(Tcl_Eval(interp, "outer"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "outer_x");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Proc with command substitution in result
// ============================================================

TEST(ProcTest, ProcResultUsedInSubstitution) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc getval {} { return hello }");
    ASSERT_EQ(Tcl_Eval(interp, "set x [getval]"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello");
    Tcl_DeleteInterp(interp);
}
