// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 8 - Regexp tests

#include "tcl.h"
#include <gtest/gtest.h>

// ============================================================
// regexp command - basic matching
// ============================================================
TEST(RegexpTest, SimpleMatch) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regexp {hel} hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(RegexpTest, NoMatch) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regexp {xyz} hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}
TEST(RegexpTest, DotMatchesAny) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regexp {h.llo} hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(RegexpTest, StarQuantifier) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regexp {hel*o} hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(RegexpTest, PlusQuantifier) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regexp {hel+o} hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(RegexpTest, Anchored) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regexp {^hello$} hello"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    ASSERT_EQ(Tcl_Eval(i, "regexp {^hello$} {hello world}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}

// ============================================================
// regexp with capture groups
// ============================================================
TEST(RegexpTest, CaptureWholeMatch) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regexp {[0-9]+} {abc123def} match"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    EXPECT_STREQ(Tcl_GetVar(i, "match", 0), "123");
    Tcl_DeleteInterp(i);
}
TEST(RegexpTest, CaptureSubgroup) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regexp {([0-9]+)_([0-9]+)} {file_12_34.txt} match g1 g2"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    EXPECT_STREQ(Tcl_GetVar(i, "match", 0), "12_34");
    EXPECT_STREQ(Tcl_GetVar(i, "g1", 0), "12");
    EXPECT_STREQ(Tcl_GetVar(i, "g2", 0), "34");
    Tcl_DeleteInterp(i);
}

// ============================================================
// regexp -nocase
// ============================================================
TEST(RegexpTest, Nocase) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regexp -nocase {hello} HELLO"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}

// ============================================================
// regsub command
// ============================================================
TEST(RegsubTest, SimpleReplace) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regsub {world} {hello world} earth"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "hello earth");
    Tcl_DeleteInterp(i);
}
TEST(RegsubTest, ReplaceToVar) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regsub {[0-9]+} {abc123} X result"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "result", 0), "abcX");
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");  // one replacement
    Tcl_DeleteInterp(i);
}
TEST(RegsubTest, ReplaceAll) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regsub -all {[0-9]} {a1b2c3} X"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "aXbXcX");
    Tcl_DeleteInterp(i);
}
TEST(RegsubTest, NoMatch) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "regsub {xyz} {hello} X result"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "result", 0), "hello");
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}

// ============================================================
// C API: Tcl_GetRegExpFromObj / Tcl_RegExpExec
// ============================================================
TEST(RegexpCApiTest, CompileAndExec) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Obj* pattern = Tcl_NewStringObj("^hello$", -1);
    Tcl_IncrRefCount(pattern);

    Tcl_RegExp re = Tcl_GetRegExpFromObj(interp, pattern, TCL_REG_ADVANCED);
    ASSERT_NE(re, nullptr);

    EXPECT_EQ(Tcl_RegExpExec(interp, re, "hello", "hello"), 1);
    EXPECT_EQ(Tcl_RegExpExec(interp, re, "world", "world"), 0);

    Tcl_DecrRefCount(pattern);
    Tcl_DeleteInterp(interp);
}
TEST(RegexpCApiTest, NocaseFlag) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Obj* pattern = Tcl_NewStringObj("^hello$", -1);
    Tcl_IncrRefCount(pattern);

    Tcl_RegExp re = Tcl_GetRegExpFromObj(interp, pattern,
                                          TCL_REG_ADVANCED | TCL_REG_NOCASE);
    ASSERT_NE(re, nullptr);
    EXPECT_EQ(Tcl_RegExpExec(interp, re, "HELLO", "HELLO"), 1);

    Tcl_DecrRefCount(pattern);
    Tcl_DeleteInterp(interp);
}
TEST(RegexpCApiTest, InvalidPattern) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Obj* pattern = Tcl_NewStringObj("[invalid", -1);
    Tcl_IncrRefCount(pattern);

    Tcl_RegExp re = Tcl_GetRegExpFromObj(interp, pattern, TCL_REG_ADVANCED);
    EXPECT_EQ(re, nullptr);

    Tcl_DecrRefCount(pattern);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Patterns used in actual OpenROAD flow scripts
// ============================================================
TEST(RegexpTest, FlowScriptPattern) {
    // Pattern from flow/scripts/util.tcl: extract stage numbers
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i,
        "regexp {/([0-9])_(([0-9])_)?} {path/1_2_synth} match num1 _ num2"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    EXPECT_STREQ(Tcl_GetVar(i, "num1", 0), "1");
    Tcl_DeleteInterp(i);
}
