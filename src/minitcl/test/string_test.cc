// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 7 - String command tests

#include "tcl.h"
#include <gtest/gtest.h>

// ============================================================
// string length
// ============================================================
TEST(StringTest, Length) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string length hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "5");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, LengthEmpty) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string length {}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string index
// ============================================================
TEST(StringTest, Index) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string index hello 1"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "e");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, IndexEnd) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string index hello end"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "o");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, IndexEndMinus) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string index hello end-1"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "l");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, IndexOutOfRange) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string index hello 99"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string range
// ============================================================
TEST(StringTest, Range) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string range hello 1 3"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "ell");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, RangeEnd) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string range hello 2 end"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "llo");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string equal / compare
// ============================================================
TEST(StringTest, Equal) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string equal abc abc"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, EqualFalse) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string equal abc def"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, Compare) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string compare abc def"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "-1");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string match
// ============================================================
TEST(StringTest, MatchExact) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string match hello hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, MatchStar) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string match {hel*} hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, MatchQuestion) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string match {h?llo} hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, MatchFail) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string match {hel*} world"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string map
// ============================================================
TEST(StringTest, Map) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string map {a A e E} hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hEllo");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, MapMultiChar) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string map {.odb .sdc} test.odb"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "test.sdc");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string trim/trimleft/trimright
// ============================================================
TEST(StringTest, Trim) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string trim {  hello  }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hello");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, TrimLeft) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string trimleft {  hello  }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hello  ");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, TrimRight) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string trimright {  hello  }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "  hello");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string tolower/toupper
// ============================================================
TEST(StringTest, ToLower) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string tolower HELLO"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hello");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, ToUpper) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string toupper hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "HELLO");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string first/last
// ============================================================
TEST(StringTest, First) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string first l hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "2");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, FirstNotFound) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string first z hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "-1");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, Last) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string last l hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "3");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string repeat
// ============================================================
TEST(StringTest, Repeat) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string repeat ab 3"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "ababab");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string is
// ============================================================
TEST(StringTest, IsInteger) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string is integer 42"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    ASSERT_EQ(Tcl_Eval(i, "string is integer abc"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, IsDouble) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string is double 3.14"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, IsAlpha) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string is alpha hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    ASSERT_EQ(Tcl_Eval(i, "string is alpha hello1"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string cat
// ============================================================
TEST(StringTest, Cat) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string cat hello { } world"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hello world");
    Tcl_DeleteInterp(i);
}

// ============================================================
// string replace
// ============================================================
TEST(StringTest, Replace) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "string replace hello 1 3 a"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hao");
    Tcl_DeleteInterp(i);
}

// ============================================================
// append
// ============================================================
TEST(StringTest, AppendToNew) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "append x hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "x", 0), "hello");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, AppendToExisting) {
    Tcl_Interp* i = Tcl_CreateInterp();
    Tcl_Eval(i, "set x hello");
    ASSERT_EQ(Tcl_Eval(i, "append x { world}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "x", 0), "hello world");
    Tcl_DeleteInterp(i);
}

// ============================================================
// format
// ============================================================
TEST(StringTest, FormatString) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "format {hello %s} world"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hello world");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, FormatInt) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "format {%d} 42"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "42");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, FormatFloat) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "format {%.2f} 3.14159"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "3.14");
    Tcl_DeleteInterp(i);
}

// ============================================================
// split
// ============================================================
TEST(StringTest, SplitDefault) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "split {a b c}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a b c");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, SplitOnChar) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "split a.b.c ."), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a b c");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, SplitEmpty) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "split abc {}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a b c");
    Tcl_DeleteInterp(i);
}

// ============================================================
// join
// ============================================================
TEST(StringTest, JoinDefault) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "join {a b c}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a b c");
    Tcl_DeleteInterp(i);
}
TEST(StringTest, JoinWithSep) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "join {a b c} ,"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "a,b,c");
    Tcl_DeleteInterp(i);
}
