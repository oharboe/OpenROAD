// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 12 - Virtual filesystem tests

#include "tcl.h"
#include "src/vfs.h"

#include <cstring>
#include <gtest/gtest.h>

TEST(VfsTest, RegisterAndGet) {
    minitcl_vfs_register("/embedded/test.tcl", "set x 42\n", 10);
    EXPECT_EQ(minitcl_vfs_exists("/embedded/test.tcl"), 1);
    size_t len = 0;
    const char *content = minitcl_vfs_get("/embedded/test.tcl", &len);
    ASSERT_NE(content, nullptr);
    EXPECT_EQ(len, 10u);
    EXPECT_STREQ(content, "set x 42\n");
}

TEST(VfsTest, NotRegistered) {
    EXPECT_EQ(minitcl_vfs_exists("/not/there"), 0);
    EXPECT_EQ(minitcl_vfs_get("/not/there", nullptr), nullptr);
}

TEST(VfsTest, SourceFromVfs) {
    minitcl_vfs_register("/vfs/init.tcl", "set vfs_loaded 1", 16);
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(interp, "source /vfs/init.tcl"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "vfs_loaded", 0), "1");
    Tcl_DeleteInterp(interp);
}

TEST(VfsTest, EvalFileFromVfs) {
    minitcl_vfs_register("/vfs/eval.tcl", "set eval_result hello", 22);
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_EvalFile(interp, "/vfs/eval.tcl"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "eval_result", 0), "hello");
    Tcl_DeleteInterp(interp);
}

TEST(VfsTest, VfsTakesPriorityOverFilesystem) {
    // Register a VFS entry that shadows any real file
    minitcl_vfs_register("/tmp/minitcl_vfs_priority.tcl", "set from vfs", 13);
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_EvalFile(interp, "/tmp/minitcl_vfs_priority.tcl"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "from", 0), "vfs");
    Tcl_DeleteInterp(interp);
}

TEST(VfsTest, VfsMultipleScripts) {
    minitcl_vfs_register("/vfs/a.tcl", "set a 1", 7);
    minitcl_vfs_register("/vfs/b.tcl", "set b 2", 7);
    Tcl_Interp* interp = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_EvalFile(interp, "/vfs/a.tcl"), TCL_OK);
    ASSERT_EQ(Tcl_EvalFile(interp, "/vfs/b.tcl"), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(interp, "a", 0), "1");
    EXPECT_STREQ(Tcl_GetVar(interp, "b", 0), "2");
    Tcl_DeleteInterp(interp);
}
