// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 14 - Misc commands tests

#include "tcl.h"
#include <cstring>
#include <ctime>
#include <gtest/gtest.h>

// ============================================================
// clock
// ============================================================
TEST(ClockTest, Seconds) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "clock seconds"), TCL_OK);
    long long val = 0;
    EXPECT_EQ(Tcl_GetWideIntFromObj(i, Tcl_GetObjResult(i), &val), TCL_OK);
    EXPECT_GT(val, 1000000000LL);  // After 2001
    Tcl_DeleteInterp(i);
}
TEST(ClockTest, Milliseconds) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "clock milliseconds"), TCL_OK);
    long long val = 0;
    EXPECT_EQ(Tcl_GetWideIntFromObj(i, Tcl_GetObjResult(i), &val), TCL_OK);
    EXPECT_GT(val, 1000000000000LL);
    Tcl_DeleteInterp(i);
}
TEST(ClockTest, Format) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "clock format 0 -format %Y"), TCL_OK);
    // Epoch year is 1970
    EXPECT_STREQ(Tcl_GetStringResult(i), "1970");
    Tcl_DeleteInterp(i);
}

// ============================================================
// exec
// ============================================================
TEST(ExecTest, SimpleCommand) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "exec echo hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hello");
    Tcl_DeleteInterp(i);
}
TEST(ExecTest, CommandWithPipe) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "exec echo abc"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "abc");
    Tcl_DeleteInterp(i);
}

// ============================================================
// package (stubs)
// ============================================================
TEST(PackageTest, Require) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "package require SomePackage"), TCL_OK);
    Tcl_DeleteInterp(i);
}
TEST(PackageTest, Provide) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "package provide MyPkg 1.0"), TCL_OK);
    Tcl_DeleteInterp(i);
}

// ============================================================
// encoding (stub)
// ============================================================
TEST(EncodingTest, System) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "encoding system"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "utf-8");
    Tcl_DeleteInterp(i);
}

// ============================================================
// lmap
// ============================================================
TEST(LmapTest, Simple) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lmap x {1 2 3} { expr {$x * 2} }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "2 4 6");
    Tcl_DeleteInterp(i);
}
TEST(LmapTest, WithString) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lmap x {a b c} { string toupper $x }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "A B C");
    Tcl_DeleteInterp(i);
}
TEST(LmapTest, Empty) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lmap x {} { expr {$x} }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "");
    Tcl_DeleteInterp(i);
}

// ============================================================
// try/finally
// ============================================================
TEST(TryTest, SuccessWithFinally) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i,
        "try { set x 1 } finally { set cleanup done }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "cleanup", 0), "done");
    Tcl_DeleteInterp(i);
}
TEST(TryTest, ErrorWithFinally) {
    Tcl_Interp* i = Tcl_CreateInterp();
    int code = Tcl_Eval(i,
        "try { error oops } finally { set cleanup done }");
    EXPECT_EQ(code, TCL_ERROR);
    EXPECT_STREQ(Tcl_GetVar(i, "cleanup", 0), "done");
    Tcl_DeleteInterp(i);
}

// ============================================================
// pwd / cd
// ============================================================
TEST(MiscTest, Pwd) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "pwd"), TCL_OK);
    EXPECT_NE(strlen(Tcl_GetStringResult(i)), 0u);
    Tcl_DeleteInterp(i);
}

// ============================================================
// pid
// ============================================================
TEST(MiscTest, Pid) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "pid"), TCL_OK);
    int val = 0;
    EXPECT_EQ(Tcl_GetInt(i, Tcl_GetStringResult(i), &val), TCL_OK);
    EXPECT_GT(val, 0);
    Tcl_DeleteInterp(i);
}

// ============================================================
// Platform variables set at startup
// ============================================================
TEST(MiscTest, TclVersion) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "set tcl_version"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "9.0");
    Tcl_DeleteInterp(i);
}
TEST(MiscTest, AutoPath) {
    Tcl_Interp* i = Tcl_CreateInterp();
    // auto_path should exist (even if empty)
    ASSERT_EQ(Tcl_Eval(i, "info exists auto_path"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
