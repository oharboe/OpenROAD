// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 3 - Eval and command registration tests

#include "tcl.h"

#include <cstring>
#include <gtest/gtest.h>

// ============================================================
// Basic eval
// ============================================================

TEST(EvalTest, EmptyScript) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(interp, ""), TCL_OK);
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, NullScript) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(interp, nullptr), TCL_OK);
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, WhitespaceScript) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(interp, "   \n  \n  "), TCL_OK);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// set command
// ============================================================

TEST(EvalTest, SetVariable) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "set x 42"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, SetAndReadVariable) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x hello");
    ASSERT_EQ(Tcl_Eval(interp, "set x"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, SetVariableWithSpaces) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "set x {hello world}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello world");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, SetVariableWithQuotes) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "set x \"hello world\""), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello world");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, SetReadNonexistent) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(interp, "set novar"), TCL_ERROR);
    // Error message should mention the variable name
    const char* err = Tcl_GetStringResult(interp);
    EXPECT_NE(strstr(err, "novar"), nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, SetOverwrite) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x first");
    Tcl_Eval(interp, "set x second");
    ASSERT_EQ(Tcl_Eval(interp, "set x"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "second");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, SetWrongArgs) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(interp, "set"), TCL_ERROR);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// unset command
// ============================================================

TEST(EvalTest, UnsetVariable) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 1");
    ASSERT_EQ(Tcl_Eval(interp, "unset x"), TCL_OK);
    EXPECT_EQ(Tcl_Eval(interp, "set x"), TCL_ERROR);
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, UnsetMultiple) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set a 1");
    Tcl_Eval(interp, "set b 2");
    ASSERT_EQ(Tcl_Eval(interp, "unset a b"), TCL_OK);
    EXPECT_EQ(Tcl_GetVar(interp, "a", 0), nullptr);
    EXPECT_EQ(Tcl_GetVar(interp, "b", 0), nullptr);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Variable substitution
// ============================================================

TEST(EvalTest, DollarSubstitution) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set name world");
    ASSERT_EQ(Tcl_Eval(interp, "set greeting \"hello $name\""), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello world");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, DollarSubstitutionBare) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 42");
    ASSERT_EQ(Tcl_Eval(interp, "set y $x"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, DollarSubstitutionBraced) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 42");
    ASSERT_EQ(Tcl_Eval(interp, "set y ${x}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, DollarSubstitutionNoBraces) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // In braced word, $ is literal
    ASSERT_EQ(Tcl_Eval(interp, "set y {$x}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "$x");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, DollarSubstitutionUndefined) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(interp, "set y $undefined"), TCL_ERROR);
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, DollarSubstitutionInQuotes) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set a hello");
    Tcl_Eval(interp, "set b world");
    ASSERT_EQ(Tcl_Eval(interp, "set c \"$a $b\""), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello world");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Command substitution
// ============================================================

TEST(EvalTest, BracketSubstitution) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x 5");
    ASSERT_EQ(Tcl_Eval(interp, "set y [set x]"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "5");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, BracketSubstitutionInQuotes) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set x hello");
    ASSERT_EQ(Tcl_Eval(interp, "set y \"value=[set x]\""), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "value=hello");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, BracketSubstitutionNoBraces) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // In braced word, [] is literal
    ASSERT_EQ(Tcl_Eval(interp, "set y {[set x]}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "[set x]");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, NestedBracketSubstitution) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Eval(interp, "set varname x");
    Tcl_Eval(interp, "set x 99");
    ASSERT_EQ(Tcl_Eval(interp, "set result [set [set varname]]"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "99");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Unknown command
// ============================================================

TEST(EvalTest, UnknownCommand) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(interp, "nosuchcmd arg1 arg2"), TCL_ERROR);
    const char* err = Tcl_GetStringResult(interp);
    EXPECT_NE(strstr(err, "nosuchcmd"), nullptr);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Command registration via C API
// ============================================================

TEST(EvalTest, CreateObjCommandAndCall) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_CreateObjCommand(
        interp, "double_it",
        [](ClientData, Tcl_Interp* interp, int objc,
           Tcl_Obj* const objv[]) -> int {
            if (objc != 2) return TCL_ERROR;
            int val = 0;
            if (Tcl_GetInt(interp, Tcl_GetString(objv[1]), &val) != TCL_OK)
                return TCL_ERROR;
            Tcl_SetObjResult(interp, Tcl_NewIntObj(val * 2));
            return TCL_OK;
        },
        nullptr, nullptr);

    ASSERT_EQ(Tcl_Eval(interp, "double_it 21"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, CreateObjCommandWithClientData) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    int counter = 0;
    Tcl_CreateObjCommand(
        interp, "increment",
        [](ClientData cd, Tcl_Interp* interp, int, Tcl_Obj* const[]) -> int {
            int* ctr = static_cast<int*>(cd);
            (*ctr)++;
            Tcl_SetObjResult(interp, Tcl_NewIntObj(*ctr));
            return TCL_OK;
        },
        &counter, nullptr);

    Tcl_Eval(interp, "increment");
    Tcl_Eval(interp, "increment");
    ASSERT_EQ(Tcl_Eval(interp, "increment"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "3");
    EXPECT_EQ(counter, 3);
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, CreateStringCommand) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_CreateCommand(
        interp, "echo",
        [](ClientData, Tcl_Interp* interp, int argc,
           const char* argv[]) -> int {
            std::string result;
            for (int i = 1; i < argc; i++) {
                if (i > 1) result += " ";
                result += argv[i];
            }
            Tcl_SetResult(interp, result.c_str(), TCL_VOLATILE);
            return TCL_OK;
        },
        nullptr, nullptr);

    ASSERT_EQ(Tcl_Eval(interp, "echo hello world"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello world");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Multiple commands in one script
// ============================================================

TEST(EvalTest, MultipleCommands) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "set x 1\nset y 2\nset z 3"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "1");
    EXPECT_STREQ(Tcl_GetVar(interp, "y", 0), "2");
    EXPECT_STREQ(Tcl_GetVar(interp, "z", 0), "3");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, SemicolonSeparatedCommands) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "set x 1; set y 2; set z 3"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "1");
    EXPECT_STREQ(Tcl_GetVar(interp, "y", 0), "2");
    EXPECT_STREQ(Tcl_GetVar(interp, "z", 0), "3");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, ErrorStopsExecution) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    // First command sets x, second fails, third should NOT run
    EXPECT_EQ(Tcl_Eval(interp, "set x 1\nnosuchcmd\nset y 2"), TCL_ERROR);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "1");
    EXPECT_EQ(Tcl_GetVar(interp, "y", 0), nullptr);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Backslash substitution in eval
// ============================================================

TEST(EvalTest, BackslashNewlineInQuotedString) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "set x \"hello\\nworld\""), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello\nworld");
    Tcl_DeleteInterp(interp);
}

TEST(EvalTest, BackslashTabInQuotedString) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "set x \"a\\tb\""), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "a\tb");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Tcl_EvalObjEx
// ============================================================

TEST(EvalTest, EvalObjEx) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Obj* obj = Tcl_NewStringObj("set x 123", -1);
    Tcl_IncrRefCount(obj);
    ASSERT_EQ(Tcl_EvalObjEx(interp, obj, 0), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "123");
    Tcl_DecrRefCount(obj);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Comments in scripts
// ============================================================

TEST(EvalTest, CommentIgnored) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "# this is a comment\nset x 1"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "1");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Result is from last command
// ============================================================

TEST(EvalTest, ResultFromLastCommand) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "set x 1\nset y 2\nset z 3"), TCL_OK);
    // Result should be from the last set command
    EXPECT_STREQ(Tcl_GetStringResult(interp), "3");
    Tcl_DeleteInterp(interp);
}
