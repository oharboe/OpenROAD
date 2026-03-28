// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 0 - Interpreter lifecycle tests

#include "tcl.h"

#include <gtest/gtest.h>

TEST(InterpTest, CreateAndDelete) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_NE(interp, nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, InitReturnsOk) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Init(interp), TCL_OK);
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, ResultInitiallyEmpty) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    EXPECT_STREQ(Tcl_GetStringResult(interp), "");
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, SetAndGetResult) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_SetResult(interp, "hello", TCL_STATIC);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello");
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, ResetResult) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_SetResult(interp, "hello", TCL_STATIC);
    Tcl_ResetResult(interp);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "");
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, AppendResult) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_AppendResult(interp, "hello", " ", "world", nullptr);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello world");
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, SetObjResult) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Obj* obj = Tcl_NewStringObj("test result", -1);
    Tcl_SetObjResult(interp, obj);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "test result");
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, AssocData) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    int data = 42;
    Tcl_SetAssocData(interp, "mydata", nullptr, &data);
    ClientData retrieved = Tcl_GetAssocData(interp, "mydata", nullptr);
    EXPECT_EQ(retrieved, &data);
    EXPECT_EQ(*static_cast<int*>(retrieved), 42);

    // Non-existent key
    EXPECT_EQ(Tcl_GetAssocData(interp, "nonexistent", nullptr), nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, SetAndGetVar) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_SetVar(interp, "x", "42", 0);
    EXPECT_STREQ(Tcl_GetVar(interp, "x", 0), "42");
    EXPECT_EQ(Tcl_GetVar(interp, "nonexistent", 0), nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, CreateObjCommand) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    auto cmd = Tcl_CreateObjCommand(
        interp, "mycommand",
        [](ClientData, Tcl_Interp* interp, int, Tcl_Obj* const[]) -> int {
            Tcl_SetResult(interp, "called", TCL_STATIC);
            return TCL_OK;
        },
        nullptr, nullptr);
    EXPECT_NE(cmd, nullptr);
    Tcl_DeleteInterp(interp);
}

TEST(InterpTest, CommandDeleteProc) {
    bool deleted = false;
    {
        Tcl_Interp* interp = Tcl_CreateInterp();
        Tcl_CreateObjCommand(
            interp, "mycommand",
            [](ClientData, Tcl_Interp*, int, Tcl_Obj* const[]) -> int {
                return TCL_OK;
            },
            &deleted,
            [](ClientData cd) { *static_cast<bool*>(cd) = true; });
        EXPECT_FALSE(deleted);
        Tcl_DeleteInterp(interp);
    }
    EXPECT_TRUE(deleted);
}
