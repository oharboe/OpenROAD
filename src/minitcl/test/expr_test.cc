// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 6 - Expression evaluator tests

#include "tcl.h"

#include <cstring>
#include <gtest/gtest.h>

// Helper to evaluate an expr and return the string result
static std::string evalExpr(Tcl_Interp* interp, const char* expr) {
    std::string cmd = "expr {";
    cmd += expr;
    cmd += "}";
    if (Tcl_Eval(interp, cmd.c_str()) != TCL_OK) {
        return std::string("ERROR: ") + Tcl_GetStringResult(interp);
    }
    return Tcl_GetStringResult(interp);
}

// ============================================================
// Integer arithmetic
// ============================================================

TEST(ExprTest, IntegerLiteral) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "42"), "42");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Addition) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "2 + 3"), "5");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Subtraction) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "10 - 3"), "7");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Multiplication) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "6 * 7"), "42");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, IntegerDivision) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "10 / 3"), "3");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Modulo) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "10 % 3"), "1");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, NegativeNumber) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "-5"), "-5");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, UnaryPlus) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "+5"), "5");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, HexLiteral) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "0xff"), "255");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Floating point arithmetic
// ============================================================

TEST(ExprTest, DoubleLiteral) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "3.14"), "3.14");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, DoubleAddition) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "1.5 + 2.5"), "4");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, DoubleDivision) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "1.0 / 4.0"), "0.25");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, MixedIntDouble) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "3 + 0.5"), "3.5");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Operator precedence
// ============================================================

TEST(ExprTest, MulBeforeAdd) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "2 + 3 * 4"), "14");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Parentheses) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "(2 + 3) * 4"), "20");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, NestedParentheses) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "((2 + 3) * (4 - 1))"), "15");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Comparison operators
// ============================================================

TEST(ExprTest, LessThan) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "3 < 5"), "1");
    EXPECT_EQ(evalExpr(interp, "5 < 3"), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, GreaterThan) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "5 > 3"), "1");
    EXPECT_EQ(evalExpr(interp, "3 > 5"), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, LessEqual) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "3 <= 3"), "1");
    EXPECT_EQ(evalExpr(interp, "4 <= 3"), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, GreaterEqual) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "3 >= 3"), "1");
    EXPECT_EQ(evalExpr(interp, "2 >= 3"), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Equal) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "5 == 5"), "1");
    EXPECT_EQ(evalExpr(interp, "5 == 6"), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, NotEqual) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "5 != 6"), "1");
    EXPECT_EQ(evalExpr(interp, "5 != 5"), "0");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// String comparison
// ============================================================

TEST(ExprTest, StringEq) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "\"hello\" eq \"hello\""), "1");
    EXPECT_EQ(evalExpr(interp, "\"hello\" eq \"world\""), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, StringNe) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "\"hello\" ne \"world\""), "1");
    EXPECT_EQ(evalExpr(interp, "\"hello\" ne \"hello\""), "0");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Logical operators
// ============================================================

TEST(ExprTest, LogicalAnd) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "1 && 1"), "1");
    EXPECT_EQ(evalExpr(interp, "1 && 0"), "0");
    EXPECT_EQ(evalExpr(interp, "0 && 1"), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, LogicalOr) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "1 || 0"), "1");
    EXPECT_EQ(evalExpr(interp, "0 || 0"), "0");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, LogicalNot) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "!1"), "0");
    EXPECT_EQ(evalExpr(interp, "!0"), "1");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Ternary operator
// ============================================================

TEST(ExprTest, TernaryTrue) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "1 ? 42 : 0"), "42");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, TernaryFalse) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "0 ? 42 : 99"), "99");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Math functions
// ============================================================

TEST(ExprTest, Abs) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "abs(-5)"), "5");
    EXPECT_EQ(evalExpr(interp, "abs(5)"), "5");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Int) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "int(3.7)"), "3");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Double) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "double(3)"), "3");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Round) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "round(3.7)"), "4");
    EXPECT_EQ(evalExpr(interp, "round(3.2)"), "3");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Sqrt) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "sqrt(4.0)"), "2");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, Pow) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "pow(2, 10)"), "1024");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, MinMax) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "min(3, 7)"), "3");
    EXPECT_EQ(evalExpr(interp, "max(3, 7)"), "7");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, CeilFloor) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "ceil(3.2)"), "4");
    EXPECT_EQ(evalExpr(interp, "floor(3.8)"), "3");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Boolean constants
// ============================================================

TEST(ExprTest, TrueFalse) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "true"), "1");
    EXPECT_EQ(evalExpr(interp, "false"), "0");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Variable substitution in expr
// ============================================================

TEST(ExprTest, VariableSubstitution) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 10");
    EXPECT_EQ(evalExpr(interp, "$x + 5"), "15");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, TwoVariables) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set a 3");
    Tcl_Eval(interp, "set b 4");
    EXPECT_EQ(evalExpr(interp, "$a * $b"), "12");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Command substitution in expr
// ============================================================

TEST(ExprTest, CommandSubstitution) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 7");
    EXPECT_EQ(evalExpr(interp, "[set x] + 3"), "10");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Power operator
// ============================================================

TEST(ExprTest, PowerOperator) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "2 ** 8"), "256");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Complex expressions
// ============================================================

TEST(ExprTest, ComplexExpr) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(evalExpr(interp, "(2 + 3) * 4 - 1"), "19");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// if/while now work with expr
// ============================================================

TEST(ExprTest, IfWithExpr) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 5");
    ASSERT_EQ(Tcl_Eval(interp, "if {$x > 3} { set result yes }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "result", 0), "yes");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, WhileWithExpr) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set i 0; set sum 0");
    ASSERT_EQ(Tcl_Eval(interp,
        "while {$i < 5} { incr sum $i; incr i }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "sum", 0), "10");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, ForWithExpr) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set sum 0");
    ASSERT_EQ(Tcl_Eval(interp,
        "for {set i 1} {$i <= 5} {incr i} { incr sum $i }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "sum", 0), "15");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, ShortCircuitAnd) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // && should short-circuit: right side not evaluated when left is false
    Tcl_Eval(interp, "proc boom {} { error {should not be called} }");
    ASSERT_EQ(Tcl_Eval(interp, "expr { 0 && [boom] }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "0");
    // Right side evaluated when left is true
    ASSERT_EQ(Tcl_Eval(interp, "expr { 1 && 1 }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "1");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, ShortCircuitOr) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // || should short-circuit: right side not evaluated when left is true
    Tcl_Eval(interp, "proc boom {} { error {should not be called} }");
    ASSERT_EQ(Tcl_Eval(interp, "expr { 1 || [boom] }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "1");
    // Right side evaluated when left is false
    ASSERT_EQ(Tcl_Eval(interp, "expr { 0 || 1 }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "1");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, TernaryShortCircuit) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "proc boom {} { error {should not be called} }");
    // True branch taken, false branch not evaluated
    ASSERT_EQ(Tcl_Eval(interp, "expr { 1 ? 42 : [boom] }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");
    // False branch taken, true branch not evaluated
    ASSERT_EQ(Tcl_Eval(interp, "expr { 0 ? [boom] : 99 }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "99");
    Tcl_DeleteInterp(interp);
}

TEST(ExprTest, ArrayVariableInExpr) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // Array access in expr should work (e.g. $::env(VAR) in ORFS scripts)
    Tcl_Eval(interp, "set arr(x) 42");
    ASSERT_EQ(Tcl_Eval(interp, "expr { $arr(x) + 1 }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "43");

    // Global array access with :: prefix
    Tcl_Eval(interp, "set ::data(key) hello");
    ASSERT_EQ(Tcl_Eval(interp, "expr { $::data(key) eq \"hello\" }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "1");

    // Array in if condition (the ORFS pattern)
    Tcl_Eval(interp, "set ::env_test(KEEP_VARS) 0");
    ASSERT_EQ(Tcl_Eval(interp,
        "proc test_if {} { if { $::env_test(KEEP_VARS) } { return yes } else { return no } }\n"
        "test_if"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "no");

    // Variable substitution inside array index
    Tcl_Eval(interp, "set idx key");
    Tcl_Eval(interp, "set myarr(key) 99");
    ASSERT_EQ(Tcl_Eval(interp, "expr { $myarr($idx) }"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "99");

    Tcl_DeleteInterp(interp);
}
