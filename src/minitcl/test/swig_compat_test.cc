// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 15 - SWIG compatibility tests
//
// These tests mimic the exact patterns that SWIG generates when wrapping
// C++ code for Tcl. They validate that minitcl provides a compatible API
// for SWIG-generated wrapper code.

#include "tcl.h"
#include <cstring>
#include <gtest/gtest.h>

// ============================================================
// SWIG pattern: Tcl_CreateObjCommand with ObjCmdProc
// ============================================================

// Simulated SWIG-generated wrapper: wraps a C++ function
// that takes (const char*, int) and returns a string
static int _wrap_myFunction(ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *const objv[]) {
    (void)clientData;
    if (objc != 3) {
        Tcl_SetResult(interp, const_cast<char*>(
            "wrong # args: should be \"myFunction name count\""),
            TCL_STATIC);
        return TCL_ERROR;
    }

    // SWIG pattern: extract arguments from Tcl_Obj
    int length;
    const char *arg1 = Tcl_GetStringFromObj(objv[1], &length);
    int arg2;
    if (Tcl_GetInt(interp, Tcl_GetString(objv[2]), &arg2) != TCL_OK) {
        return TCL_ERROR;
    }

    // Call "C++ function" and return result
    // (simulated: just repeat the name arg2 times)
    std::string result;
    for (int i = 0; i < arg2; i++) {
        if (i > 0) result += " ";
        result += arg1;
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
    return TCL_OK;
}

TEST(SwigCompatTest, RegisterAndCallWrappedFunction) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_CreateObjCommand(interp, "myFunction", _wrap_myFunction,
                          nullptr, nullptr);
    ASSERT_EQ(Tcl_Eval(interp, "myFunction hello 3"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello hello hello");
    Tcl_DeleteInterp(interp);
}

TEST(SwigCompatTest, WrongArgCount) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_CreateObjCommand(interp, "myFunction", _wrap_myFunction,
                          nullptr, nullptr);
    EXPECT_EQ(Tcl_Eval(interp, "myFunction"), TCL_ERROR);
    EXPECT_NE(strstr(Tcl_GetStringResult(interp), "wrong # args"), nullptr);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: Tcl_SetResult with TCL_VOLATILE
// ============================================================

TEST(SwigCompatTest, SetResultVolatile) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    {
        std::string temp = "temporary result";
        Tcl_SetResult(interp, temp.c_str(), TCL_VOLATILE);
    }
    // String should have been copied
    EXPECT_STREQ(Tcl_GetStringResult(interp), "temporary result");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: Tcl_SetResult with TCL_STATIC
// ============================================================

TEST(SwigCompatTest, SetResultStatic) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    static const char *msg = "static message";
    Tcl_SetResult(interp, msg, TCL_STATIC);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "static message");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: building return lists with Tcl_NewListObj
// ============================================================

TEST(SwigCompatTest, BuildReturnList) {
    Tcl_Interp* interp = Tcl_CreateInterp();

    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    Tcl_IncrRefCount(list);

    Tcl_ListObjAppendElement(interp, list, Tcl_NewStringObj("name", -1));
    Tcl_ListObjAppendElement(interp, list, Tcl_NewIntObj(42));
    Tcl_ListObjAppendElement(interp, list, Tcl_NewDoubleObj(3.14));

    Tcl_SetObjResult(interp, list);

    // Verify the list elements can be extracted
    Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
    int objc;
    Tcl_Obj **objv;
    ASSERT_EQ(Tcl_ListObjGetElements(interp, resultObj, &objc, &objv), TCL_OK);
    EXPECT_EQ(objc, 3);
    EXPECT_STREQ(Tcl_GetString(objv[0]), "name");
    EXPECT_STREQ(Tcl_GetString(objv[1]), "42");

    Tcl_DecrRefCount(list);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: parsing argument lists from Tcl_Obj
// ============================================================

TEST(SwigCompatTest, ParseListArgument) {
    Tcl_Interp* interp = Tcl_CreateInterp();

    // Register a command that takes a list argument
    Tcl_CreateObjCommand(interp, "sumList",
        [](ClientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const objv[]) -> int {
            if (objc != 2) return TCL_ERROR;
            int listLen;
            Tcl_Obj **listElems;
            if (Tcl_ListObjGetElements(interp, objv[1], &listLen, &listElems)
                != TCL_OK)
                return TCL_ERROR;
            int sum = 0;
            for (int i = 0; i < listLen; i++) {
                int val;
                if (Tcl_GetInt(interp, Tcl_GetString(listElems[i]), &val)
                    != TCL_OK)
                    return TCL_ERROR;
                sum += val;
            }
            Tcl_SetObjResult(interp, Tcl_NewIntObj(sum));
            return TCL_OK;
        },
        nullptr, nullptr);

    ASSERT_EQ(Tcl_Eval(interp, "sumList {1 2 3 4 5}"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "15");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: Tcl_GetDoubleFromObj for numeric args
// ============================================================

TEST(SwigCompatTest, GetDoubleFromObjArg) {
    Tcl_Interp* interp = Tcl_CreateInterp();

    Tcl_CreateObjCommand(interp, "area",
        [](ClientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const objv[]) -> int {
            if (objc != 3) return TCL_ERROR;
            double w, h;
            if (Tcl_GetDoubleFromObj(interp, objv[1], &w) != TCL_OK)
                return TCL_ERROR;
            if (Tcl_GetDoubleFromObj(interp, objv[2], &h) != TCL_OK)
                return TCL_ERROR;
            Tcl_SetObjResult(interp, Tcl_NewDoubleObj(w * h));
            return TCL_OK;
        },
        nullptr, nullptr);

    ASSERT_EQ(Tcl_Eval(interp, "area 3.0 4.0"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "12");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: Tcl_ResetResult + Tcl_AppendResult for errors
// ============================================================

TEST(SwigCompatTest, ResetAndAppendResult) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "Error in ", "function", ": ",
                      "bad argument", nullptr);
    EXPECT_STREQ(Tcl_GetStringResult(interp),
                  "Error in function: bad argument");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: Tcl_SetAssocData / Tcl_GetAssocData
// (used for storing Design* in OpenROAD)
// ============================================================

struct FakeDesign {
    int id;
    const char *name;
};

TEST(SwigCompatTest, AssocDataForDesign) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    FakeDesign design = {42, "gcd"};
    Tcl_SetAssocData(interp, "design", nullptr, &design);

    // Later, in a command handler, retrieve it
    ClientData cd = Tcl_GetAssocData(interp, "design", nullptr);
    auto *retrieved = static_cast<FakeDesign *>(cd);
    EXPECT_EQ(retrieved->id, 42);
    EXPECT_STREQ(retrieved->name, "gcd");

    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: Module init function
// ============================================================

static int MyModule_Init(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "my_greet",
        [](ClientData, Tcl_Interp *interp, int, Tcl_Obj *const[]) -> int {
            Tcl_SetResult(interp, const_cast<char*>("hello from module"),
                           TCL_STATIC);
            return TCL_OK;
        },
        nullptr, nullptr);
    return TCL_OK;
}

TEST(SwigCompatTest, ModuleInit) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(MyModule_Init(interp), TCL_OK);
    ASSERT_EQ(Tcl_Eval(interp, "my_greet"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "hello from module");
    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: Tcl_Eval for init script (evalTclInit pattern)
// ============================================================

TEST(SwigCompatTest, EvalInitScript) {
    Tcl_Interp* interp = Tcl_CreateInterp();

    // Simulates what evalTclInit does: eval a multi-command init script
    const char *initScript =
        "proc my_helper {x} { return [expr {$x + 1}] }\n"
        "proc my_main {} { return [my_helper 41] }\n";

    ASSERT_EQ(Tcl_Eval(interp, initScript), TCL_OK);
    ASSERT_EQ(Tcl_Eval(interp, "my_main"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "42");

    Tcl_DeleteInterp(interp);
}

// ============================================================
// SWIG pattern: Tcl_Obj ref counting in error paths
// ============================================================

TEST(SwigCompatTest, RefCountInErrorPath) {
    Tcl_Interp* interp = Tcl_CreateInterp();

    // Create objects, incr ref, then clean up even on error
    Tcl_Obj *obj = Tcl_NewStringObj("test", -1);
    Tcl_IncrRefCount(obj);
    EXPECT_EQ(obj->refCount, 1);

    // Simulate error path where we still need to clean up
    Tcl_SetResult(interp, const_cast<char*>("error"), TCL_STATIC);
    Tcl_DecrRefCount(obj);  // Should free the object

    Tcl_DeleteInterp(interp);
}
