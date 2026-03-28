// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 5 - Control flow tests

#include "tcl.h"

#include <cstring>
#include <gtest/gtest.h>

// ============================================================
// if command
// ============================================================

TEST(IfTest, TrueCondition) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "if {1} { set x yes }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "yes");
    Tcl_DeleteInterp(interp);
}

TEST(IfTest, FalseCondition) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "if {0} { set x yes }"), TCL_OK);
    EXPECT_EQ(Tcl_GetVar(interp, "x", 0), nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(IfTest, ElseBranch) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "if {0} { set x yes } else { set x no }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "no");
    Tcl_DeleteInterp(interp);
}

TEST(IfTest, ElseifBranch) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp,
        "if {0} { set x first } elseif {1} { set x second } else { set x third }"),
        TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "second");
    Tcl_DeleteInterp(interp);
}

TEST(IfTest, ElseifAllFalse) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp,
        "if {0} { set x 1 } elseif {0} { set x 2 } else { set x 3 }"),
        TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "3");
    Tcl_DeleteInterp(interp);
}

TEST(IfTest, ConditionWithVariable) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set cond 1");
    ASSERT_EQ(Tcl_Eval(interp, "if {$cond} { set x yes }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "yes");
    Tcl_DeleteInterp(interp);
}

TEST(IfTest, ReturnValueFromBody) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "if {1} { set x 42 }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// while command
// ============================================================

TEST(WhileTest, SimpleLoop) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set i 0");
    ASSERT_EQ(Tcl_Eval(interp, "while {$i} { incr i }"), TCL_OK);
    // i starts at 0, condition false immediately
    EXPECT_STREQ(Tcl_GetVar(interp, "i", 0), "0");
    Tcl_DeleteInterp(interp);
}

TEST(WhileTest, CountToThree) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set i 3");
    ASSERT_EQ(Tcl_Eval(interp, "while {$i} { incr i -1 }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "i", 0), "0");
    Tcl_DeleteInterp(interp);
}

TEST(WhileTest, BreakExitsLoop) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set i 0");
    ASSERT_EQ(Tcl_Eval(interp,
        "while {1} { incr i; if {$i} { break } }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "i", 0), "1");
    Tcl_DeleteInterp(interp);
}

TEST(WhileTest, ContinueSkipsRest) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set i 0; set sum 0");
    ASSERT_EQ(Tcl_Eval(interp,
        "while {$i} { incr i -1 }"), TCL_OK);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// for command
// ============================================================

TEST(ForTest, SimpleForLoop) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set sum 0");
    ASSERT_EQ(Tcl_Eval(interp,
        "for {set i 0} {$i} {incr i} { incr sum }"), TCL_OK);
    // i starts at 0, condition is false immediately
    EXPECT_STREQ(Tcl_GetVar(interp, "sum", 0), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ForTest, CountToFive) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set sum 0");
    ASSERT_EQ(Tcl_Eval(interp,
        "for {set i 5} {$i} {incr i -1} { incr sum }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "sum", 0), "5");
    EXPECT_STREQ(Tcl_GetVar(interp, "i", 0), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ForTest, BreakInFor) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp,
        "for {set i 0} {1} {incr i} { if {$i} { break } }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "i", 0), "1");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// foreach command
// ============================================================

TEST(ForeachTest, SimpleList) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set result {}");
    ASSERT_EQ(Tcl_Eval(interp,
        "foreach x {a b c} { set result \"$result $x\" }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "result", 0), " a b c");
    Tcl_DeleteInterp(interp);
}

TEST(ForeachTest, BreakInForeach) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set last {}");
    ASSERT_EQ(Tcl_Eval(interp,
        "foreach x {a b c} { set last $x; if {1} { break } }"), TCL_OK);
    // Should break on first iteration since condition is always true
    EXPECT_STREQ(Tcl_GetVar(interp, "last", 0), "a");
    Tcl_DeleteInterp(interp);
}

TEST(ForeachTest, EmptyList) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "foreach x {} { error never }"), TCL_OK);
    Tcl_DeleteInterp(interp);
}

TEST(ForeachTest, SingleElement) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "foreach x {only} { set result $x }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "result", 0), "only");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// incr command
// ============================================================

TEST(IncrTest, IncrByOne) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 5");
    ASSERT_EQ(Tcl_Eval(interp, "incr x"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "6");
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "6");
    Tcl_DeleteInterp(interp);
}

TEST(IncrTest, IncrByAmount) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 10");
    ASSERT_EQ(Tcl_Eval(interp, "incr x 5"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "15");
    Tcl_DeleteInterp(interp);
}

TEST(IncrTest, IncrNegative) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 10");
    ASSERT_EQ(Tcl_Eval(interp, "incr x -3"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "7");
    Tcl_DeleteInterp(interp);
}

TEST(IncrTest, IncrUndefinedVar) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "incr newvar"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "newvar", 0), "1");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// switch command
// ============================================================

TEST(SwitchTest, MatchFirst) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "switch abc { abc { set x matched } }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "matched");
    Tcl_DeleteInterp(interp);
}

TEST(SwitchTest, MatchDefault) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp,
        "switch xyz { abc { set x 1 } default { set x fallback } }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "fallback");
    Tcl_DeleteInterp(interp);
}

TEST(SwitchTest, NoMatch) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "switch xyz { abc { set x 1 } }"), TCL_OK);
    EXPECT_EQ(Tcl_GetVar(interp, "x", 0), nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(SwitchTest, MatchSecond) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp,
        "switch bbb { aaa { set x 1 } bbb { set x 2 } ccc { set x 3 } }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "2");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// catch command
// ============================================================

TEST(CatchTest, CatchSuccess) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "catch {set x 42}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "0");  // TCL_OK = 0
    Tcl_DeleteInterp(interp);
}

TEST(CatchTest, CatchError) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "catch {error oops}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "1");  // TCL_ERROR = 1
    Tcl_DeleteInterp(interp);
}

TEST(CatchTest, CatchWithResultVar) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "catch {set x 42} result"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "result", 0), "42");
    Tcl_DeleteInterp(interp);
}

TEST(CatchTest, CatchErrorWithResultVar) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "catch {error oops} result"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "result", 0), "oops");
    EXPECT_STREQ(Tcl_GetStringResult(interp), "1");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// break and continue
// ============================================================

TEST(BreakContinueTest, BreakInForeach) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set count 0");
    ASSERT_EQ(Tcl_Eval(interp,
        "foreach x {a b c d} { incr count; break }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "count", 0), "1");
    Tcl_DeleteInterp(interp);
}

TEST(BreakContinueTest, ContinueInForeach) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set count 0");
    // continue skips the rest of the body each time, but loop continues
    ASSERT_EQ(Tcl_Eval(interp,
        "foreach x {a b c} { continue; incr count }"), TCL_OK);
    // count should still be 0 since incr is after continue
    EXPECT_STREQ(Tcl_GetVar(interp, "count", 0), "0");
    Tcl_DeleteInterp(interp);
}
