// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 7 - List command tests

#include "tcl.h"
#include <gtest/gtest.h>

// ============================================================
// list
// ============================================================
TEST(ListCmdTest, ListEmpty) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "list"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, ListSimple) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "list a b c"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a b c");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, ListWithSpaces) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "list {hello world} foo"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "{hello world} foo");
    Tcl_DeleteInterp(i);
}

// ============================================================
// llength
// ============================================================
TEST(ListCmdTest, LlengthEmpty) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "llength {}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LlengthThree) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "llength {a b c}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "3");
    Tcl_DeleteInterp(i);
}

// ============================================================
// lindex
// ============================================================
TEST(ListCmdTest, LindexFirst) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lindex {a b c} 0"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LindexLast) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lindex {a b c} end"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "c");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LindexOutOfRange) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lindex {a b c} 99"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "");
    Tcl_DeleteInterp(i);
}

// ============================================================
// lrange
// ============================================================
TEST(ListCmdTest, LrangeSubset) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lrange {a b c d e} 1 3"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "b c d");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LrangeEnd) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lrange {a b c d e} 2 end"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "c d e");
    Tcl_DeleteInterp(i);
}

// ============================================================
// lappend
// ============================================================
TEST(ListCmdTest, LappendNew) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lappend mylist a"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "mylist", 0), "a");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LappendExisting) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set mylist {a b}");
    ASSERT_EQ(Tcl_Eval(i, "lappend mylist c d"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "mylist", 0), "a b c d");
    Tcl_DeleteInterp(i);
}

// ============================================================
// lsort
// ============================================================
TEST(ListCmdTest, LsortAlpha) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lsort {c a b}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a b c");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LsortDecreasing) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lsort -decreasing {a b c}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "c b a");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LsortDictionary) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lsort -dictionary {Banana apple Cherry}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "apple Banana Cherry");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LsortInteger) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lsort -integer {10 2 30 1}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1 2 10 30");
    Tcl_DeleteInterp(i);
}

// ============================================================
// lsearch
// ============================================================
TEST(ListCmdTest, LsearchFound) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lsearch {a b c} b"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LsearchNotFound) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lsearch {a b c} z"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "-1");
    Tcl_DeleteInterp(i);
}

// ============================================================
// concat
// ============================================================
TEST(ListCmdTest, Concat) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "concat {a b} {c d}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a b c d");
    Tcl_DeleteInterp(i);
}

// ============================================================
// lassign
// ============================================================
TEST(ListCmdTest, Lassign) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lassign {10 20} x y"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "x", 0), "10");
    EXPECT_STREQ(Tcl_GetVar(i, "y", 0), "20");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LassignExtraVars) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lassign {10} x y"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "x", 0), "10");
    EXPECT_STREQ(Tcl_GetVar(i, "y", 0), "");
    Tcl_DeleteInterp(i);
}
TEST(ListCmdTest, LassignExtraElems) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "lassign {10 20 30} x"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "x", 0), "10");
    EXPECT_STREQ(Tcl_GetStringResult(i), "20 30");
    Tcl_DeleteInterp(i);
}
