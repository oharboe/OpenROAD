// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 10 - Dict, array, info tests

#include "tcl.h"
#include <cstring>
#include <gtest/gtest.h>

// ============================================================
// dict create/get/set/keys/values/exists/size
// ============================================================
TEST(DictTest, Create) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "dict create a 1 b 2"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a 1 b 2");
    Tcl_DeleteInterp(i);
}
TEST(DictTest, Get) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "dict get {a 1 b 2} b"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "2");
    Tcl_DeleteInterp(i);
}
TEST(DictTest, GetMissing) {
    Tcl_Interp* i = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(i, "dict get {a 1} z"), TCL_ERROR);
    Tcl_DeleteInterp(i);
}
TEST(DictTest, SetNewKey) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set d {a 1}");
    ASSERT_EQ(Tcl_Eval(i, "dict set d b 2"), TCL_OK);
    ASSERT_EQ(Tcl_Eval(i, "dict get $d b"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "2");
    Tcl_DeleteInterp(i);
}
TEST(DictTest, SetOverwrite) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set d {a 1 b 2}");
    ASSERT_EQ(Tcl_Eval(i, "dict set d a 99"), TCL_OK);
    ASSERT_EQ(Tcl_Eval(i, "dict get $d a"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "99");
    Tcl_DeleteInterp(i);
}
TEST(DictTest, Keys) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "dict keys {a 1 b 2 c 3}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a b c");
    Tcl_DeleteInterp(i);
}
TEST(DictTest, Values) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "dict values {a 1 b 2}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1 2");
    Tcl_DeleteInterp(i);
}
TEST(DictTest, Exists) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "dict exists {a 1 b 2} a"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    ASSERT_EQ(Tcl_Eval(i, "dict exists {a 1 b 2} z"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}
TEST(DictTest, Size) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "dict size {a 1 b 2 c 3}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "3");
    Tcl_DeleteInterp(i);
}
TEST(DictTest, For) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set result {}");
    ASSERT_EQ(Tcl_Eval(i,
        "dict for {k v} {a 1 b 2} { append result $k=$v, }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "result", 0), "a=1,b=2,");
    Tcl_DeleteInterp(i);
}

// ============================================================
// array
// ============================================================
TEST(ArrayTest, SetAndGet) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "array set arr {x 1 y 2}");
    ASSERT_EQ(Tcl_Eval(i, "set arr(x)"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(ArrayTest, Exists) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set arr(x) 1");
    ASSERT_EQ(Tcl_Eval(i, "array exists arr"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    ASSERT_EQ(Tcl_Eval(i, "array exists noarr"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}
TEST(ArrayTest, Names) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set arr(x) 1; set arr(y) 2");
    ASSERT_EQ(Tcl_Eval(i, "array names arr"), TCL_OK);
    // Order may vary, just check both are present
    std::string names = Tcl_GetStringResult(i);
    EXPECT_NE(names.find("x"), std::string::npos);
    EXPECT_NE(names.find("y"), std::string::npos);
    Tcl_DeleteInterp(i);
}
TEST(ArrayTest, Size) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set arr(a) 1; set arr(b) 2; set arr(c) 3");
    ASSERT_EQ(Tcl_Eval(i, "array size arr"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "3");
    Tcl_DeleteInterp(i);
}

// ============================================================
// info
// ============================================================
TEST(InfoTest, ExistsTrue) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set x 1");
    ASSERT_EQ(Tcl_Eval(i, "info exists x"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(InfoTest, ExistsFalse) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "info exists novar"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}
TEST(InfoTest, Commands) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "info commands"), TCL_OK);
    std::string cmds = Tcl_GetStringResult(i);
    EXPECT_NE(cmds.find("set"), std::string::npos);
    EXPECT_NE(cmds.find("if"), std::string::npos);
    Tcl_DeleteInterp(i);
}
TEST(InfoTest, Procs) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc myf {} {}");
    ASSERT_EQ(Tcl_Eval(i, "info procs"), TCL_OK);
    EXPECT_NE(std::string(Tcl_GetStringResult(i)).find("myf"), std::string::npos);
    Tcl_DeleteInterp(i);
}
TEST(InfoTest, Body) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc myf {x} { return $x }");
    ASSERT_EQ(Tcl_Eval(i, "info body myf"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), " return $x ");
    Tcl_DeleteInterp(i);
}
TEST(InfoTest, Args) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc myf {a b c} {}");
    ASSERT_EQ(Tcl_Eval(i, "info args myf"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a b c");
    Tcl_DeleteInterp(i);
}
TEST(InfoTest, Level) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "info level"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}
TEST(InfoTest, LevelInProc) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "proc f {} { info level }");
    ASSERT_EQ(Tcl_Eval(i, "f"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
