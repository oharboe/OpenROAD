// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 1 - Object system and result handling tests

#include "tcl.h"

#include <cstring>
#include <gtest/gtest.h>

// ============================================================
// String objects
// ============================================================

TEST(ObjTest, NewStringObj) {
    Tcl_Obj* obj = Tcl_NewStringObj("hello", 5);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->refCount, 0);
    EXPECT_STREQ(obj->bytes, "hello");
    EXPECT_EQ(obj->length, 5);
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, NewStringObjNegativeLength) {
    Tcl_Obj* obj = Tcl_NewStringObj("hello world", -1);
    EXPECT_STREQ(obj->bytes, "hello world");
    EXPECT_EQ(obj->length, 11);
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, NewStringObjNull) {
    Tcl_Obj* obj = Tcl_NewStringObj(nullptr, 0);
    EXPECT_STREQ(Tcl_GetString(obj), "");
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, NewObj) {
    Tcl_Obj* obj = Tcl_NewObj();
    EXPECT_STREQ(Tcl_GetString(obj), "");
    EXPECT_EQ(obj->length, 0);
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, GetString) {
    Tcl_Obj* obj = Tcl_NewStringObj("test", -1);
    EXPECT_STREQ(Tcl_GetString(obj), "test");
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, GetStringFromObj) {
    Tcl_Obj* obj = Tcl_NewStringObj("test", -1);
    int length = 0;
    const char* s = Tcl_GetStringFromObj(obj, &length);
    EXPECT_STREQ(s, "test");
    EXPECT_EQ(length, 4);
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, GetStringFromObjNullLength) {
    Tcl_Obj* obj = Tcl_NewStringObj("abc", -1);
    const char* s = Tcl_GetStringFromObj(obj, nullptr);
    EXPECT_STREQ(s, "abc");
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

// ============================================================
// Integer objects
// ============================================================

TEST(ObjTest, NewIntObj) {
    Tcl_Obj* obj = Tcl_NewIntObj(42);
    EXPECT_STREQ(Tcl_GetString(obj), "42");
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, NewIntObjNegative) {
    Tcl_Obj* obj = Tcl_NewIntObj(-7);
    EXPECT_STREQ(Tcl_GetString(obj), "-7");
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, NewIntObjZero) {
    Tcl_Obj* obj = Tcl_NewIntObj(0);
    EXPECT_STREQ(Tcl_GetString(obj), "0");
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, SetIntObj) {
    Tcl_Obj* obj = Tcl_NewIntObj(1);
    Tcl_SetIntObj(obj, 99);
    EXPECT_STREQ(Tcl_GetString(obj), "99");
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, GetInt) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    int val = 0;
    EXPECT_EQ(Tcl_GetInt(interp, "123", &val), TCL_OK);
    EXPECT_EQ(val, 123);

    EXPECT_EQ(Tcl_GetInt(interp, "-42", &val), TCL_OK);
    EXPECT_EQ(val, -42);

    EXPECT_EQ(Tcl_GetInt(interp, "abc", &val), TCL_ERROR);
    Tcl_DeleteInterp(interp);
}

TEST(ObjTest, GetIntHex) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    int val = 0;
    EXPECT_EQ(Tcl_GetInt(interp, "0xff", &val), TCL_OK);
    EXPECT_EQ(val, 255);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Double objects
// ============================================================

TEST(ObjTest, NewDoubleObj) {
    Tcl_Obj* obj = Tcl_NewDoubleObj(3.14);
    // snprintf %g format
    double val = 0;
    EXPECT_EQ(Tcl_GetDoubleFromObj(nullptr, obj, &val), TCL_OK);
    EXPECT_DOUBLE_EQ(val, 3.14);
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, GetDouble) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    double val = 0;
    EXPECT_EQ(Tcl_GetDouble(interp, "2.718", &val), TCL_OK);
    EXPECT_DOUBLE_EQ(val, 2.718);

    EXPECT_EQ(Tcl_GetDouble(interp, "notanumber", &val), TCL_ERROR);
    Tcl_DeleteInterp(interp);
}

TEST(ObjTest, GetDoubleFromObj) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Obj* obj = Tcl_NewStringObj("1.5", -1);
    double val = 0;
    EXPECT_EQ(Tcl_GetDoubleFromObj(interp, obj, &val), TCL_OK);
    EXPECT_DOUBLE_EQ(val, 1.5);
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
    Tcl_DeleteInterp(interp);
}

// ============================================================
// Wide int objects
// ============================================================

TEST(ObjTest, NewWideIntObj) {
    Tcl_Obj* obj = Tcl_NewWideIntObj(1234567890123LL);
    long long val = 0;
    EXPECT_EQ(Tcl_GetWideIntFromObj(nullptr, obj, &val), TCL_OK);
    EXPECT_EQ(val, 1234567890123LL);
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

// ============================================================
// Boolean objects
// ============================================================

TEST(ObjTest, NewBooleanObj) {
    Tcl_Obj* obj = Tcl_NewBooleanObj(1);
    EXPECT_STREQ(Tcl_GetString(obj), "1");
    Tcl_IncrRefCount(obj);
    Tcl_DecrRefCount(obj);
}

TEST(ObjTest, GetBooleanFromObj) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    int val = -1;

    Tcl_Obj* obj_true = Tcl_NewStringObj("true", -1);
    EXPECT_EQ(Tcl_GetBooleanFromObj(interp, obj_true, &val), TCL_OK);
    EXPECT_EQ(val, 1);
    Tcl_IncrRefCount(obj_true);
    Tcl_DecrRefCount(obj_true);

    Tcl_Obj* obj_false = Tcl_NewStringObj("false", -1);
    EXPECT_EQ(Tcl_GetBooleanFromObj(interp, obj_false, &val), TCL_OK);
    EXPECT_EQ(val, 0);
    Tcl_IncrRefCount(obj_false);
    Tcl_DecrRefCount(obj_false);

    Tcl_Obj* obj_yes = Tcl_NewStringObj("yes", -1);
    EXPECT_EQ(Tcl_GetBooleanFromObj(interp, obj_yes, &val), TCL_OK);
    EXPECT_EQ(val, 1);
    Tcl_IncrRefCount(obj_yes);
    Tcl_DecrRefCount(obj_yes);

    Tcl_Obj* obj_bad = Tcl_NewStringObj("maybe", -1);
    EXPECT_EQ(Tcl_GetBooleanFromObj(interp, obj_bad, &val), TCL_ERROR);
    Tcl_IncrRefCount(obj_bad);
    Tcl_DecrRefCount(obj_bad);

    Tcl_DeleteInterp(interp);
}

// ============================================================
// Reference counting
// ============================================================

TEST(ObjTest, RefCountIncDecr) {
    Tcl_Obj* obj = Tcl_NewStringObj("test", -1);
    EXPECT_EQ(obj->refCount, 0);
    Tcl_IncrRefCount(obj);
    EXPECT_EQ(obj->refCount, 1);
    Tcl_IncrRefCount(obj);
    EXPECT_EQ(obj->refCount, 2);
    Tcl_DecrRefCount(obj);
    EXPECT_EQ(obj->refCount, 1);
    Tcl_DecrRefCount(obj);
    // obj is freed here - don't access it
}

// ============================================================
// List objects
// ============================================================

TEST(ObjTest, NewListObjEmpty) {
    Tcl_Obj* list = Tcl_NewListObj(0, nullptr);
    EXPECT_STREQ(Tcl_GetString(list), "");
    int objc = -1;
    Tcl_Obj** objv = nullptr;
    EXPECT_EQ(Tcl_ListObjGetElements(nullptr, list, &objc, &objv), TCL_OK);
    EXPECT_EQ(objc, 0);
    Tcl_IncrRefCount(list);
    Tcl_DecrRefCount(list);
}

TEST(ObjTest, NewListObjWithElements) {
    Tcl_Obj* elems[3];
    elems[0] = Tcl_NewStringObj("a", -1);
    elems[1] = Tcl_NewStringObj("b", -1);
    elems[2] = Tcl_NewStringObj("c", -1);
    Tcl_Obj* list = Tcl_NewListObj(3, elems);

    EXPECT_STREQ(Tcl_GetString(list), "a b c");

    int objc = 0;
    Tcl_Obj** objv = nullptr;
    EXPECT_EQ(Tcl_ListObjGetElements(nullptr, list, &objc, &objv), TCL_OK);
    EXPECT_EQ(objc, 3);
    EXPECT_STREQ(Tcl_GetString(objv[0]), "a");
    EXPECT_STREQ(Tcl_GetString(objv[1]), "b");
    EXPECT_STREQ(Tcl_GetString(objv[2]), "c");

    Tcl_IncrRefCount(list);
    Tcl_DecrRefCount(list);
}

TEST(ObjTest, ListObjAppendElement) {
    Tcl_Obj* list = Tcl_NewListObj(0, nullptr);
    Tcl_IncrRefCount(list);

    Tcl_Obj* elem1 = Tcl_NewStringObj("hello", -1);
    EXPECT_EQ(Tcl_ListObjAppendElement(nullptr, list, elem1), TCL_OK);

    Tcl_Obj* elem2 = Tcl_NewStringObj("world", -1);
    EXPECT_EQ(Tcl_ListObjAppendElement(nullptr, list, elem2), TCL_OK);

    EXPECT_STREQ(Tcl_GetString(list), "hello world");

    int objc = 0;
    Tcl_Obj** objv = nullptr;
    Tcl_ListObjGetElements(nullptr, list, &objc, &objv);
    EXPECT_EQ(objc, 2);

    Tcl_DecrRefCount(list);
}

TEST(ObjTest, ListObjWithSpacesQuoted) {
    Tcl_Obj* elems[2];
    elems[0] = Tcl_NewStringObj("hello world", -1);
    elems[1] = Tcl_NewStringObj("foo", -1);
    Tcl_Obj* list = Tcl_NewListObj(2, elems);

    // "hello world" should be braced
    EXPECT_STREQ(Tcl_GetString(list), "{hello world} foo");

    Tcl_IncrRefCount(list);
    Tcl_DecrRefCount(list);
}

TEST(ObjTest, ListObjLength) {
    Tcl_Obj* elems[2];
    elems[0] = Tcl_NewStringObj("a", -1);
    elems[1] = Tcl_NewStringObj("b", -1);
    Tcl_Obj* list = Tcl_NewListObj(2, elems);

    int length = 0;
    EXPECT_EQ(Tcl_ListObjLength(nullptr, list, &length), TCL_OK);
    EXPECT_EQ(length, 2);

    Tcl_IncrRefCount(list);
    Tcl_DecrRefCount(list);
}

TEST(ObjTest, ListObjIndex) {
    Tcl_Obj* elems[3];
    elems[0] = Tcl_NewStringObj("x", -1);
    elems[1] = Tcl_NewStringObj("y", -1);
    elems[2] = Tcl_NewStringObj("z", -1);
    Tcl_Obj* list = Tcl_NewListObj(3, elems);

    Tcl_Obj* item = nullptr;
    EXPECT_EQ(Tcl_ListObjIndex(nullptr, list, 1, &item), TCL_OK);
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(Tcl_GetString(item), "y");

    // Out of range
    EXPECT_EQ(Tcl_ListObjIndex(nullptr, list, 5, &item), TCL_OK);
    EXPECT_EQ(item, nullptr);

    Tcl_IncrRefCount(list);
    Tcl_DecrRefCount(list);
}

// ============================================================
// DString
// ============================================================

TEST(ObjTest, DStringBasic) {
    Tcl_DString ds;
    Tcl_DStringInit(&ds);
    EXPECT_EQ(Tcl_DStringLength(&ds), 0);
    EXPECT_STREQ(Tcl_DStringValue(&ds), "");

    Tcl_DStringAppend(&ds, "hello", 5);
    EXPECT_EQ(Tcl_DStringLength(&ds), 5);
    EXPECT_STREQ(Tcl_DStringValue(&ds), "hello");

    Tcl_DStringAppend(&ds, " world", -1);
    EXPECT_EQ(Tcl_DStringLength(&ds), 11);
    EXPECT_STREQ(Tcl_DStringValue(&ds), "hello world");

    Tcl_DStringFree(&ds);
    EXPECT_EQ(Tcl_DStringLength(&ds), 0);
}

TEST(ObjTest, DStringLargeAppend) {
    Tcl_DString ds;
    Tcl_DStringInit(&ds);
    // Append enough to overflow static space (200 bytes)
    for (int i = 0; i < 50; i++) {
        Tcl_DStringAppend(&ds, "abcde", 5);
    }
    EXPECT_EQ(Tcl_DStringLength(&ds), 250);
    EXPECT_EQ(strlen(Tcl_DStringValue(&ds)), 250u);
    Tcl_DStringFree(&ds);
}

// ============================================================
// Result handling via Obj
// ============================================================

TEST(ObjTest, SetAndGetObjResult) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Obj* obj = Tcl_NewStringObj("result value", -1);
    Tcl_SetObjResult(interp, obj);

    Tcl_Obj* result = Tcl_GetObjResult(interp);
    EXPECT_STREQ(Tcl_GetString(result), "result value");

    // GetStringResult should also work
    EXPECT_STREQ(Tcl_GetStringResult(interp), "result value");

    Tcl_DeleteInterp(interp);
}

TEST(ObjTest, ResetResultClearsObj) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_SetObjResult(interp, Tcl_NewStringObj("something", -1));
    Tcl_ResetResult(interp);
    EXPECT_STREQ(Tcl_GetStringResult(interp), "");
    Tcl_DeleteInterp(interp);
}

TEST(ObjTest, GetReturnOptions) {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Obj* opts = Tcl_GetReturnOptions(interp, TCL_OK);
    ASSERT_NE(opts, nullptr);
    // Should be an empty list for now
    Tcl_IncrRefCount(opts);
    Tcl_DecrRefCount(opts);
    Tcl_DeleteInterp(interp);
}
