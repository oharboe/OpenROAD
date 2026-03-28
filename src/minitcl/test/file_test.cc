// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 11 - File and I/O tests

#include "tcl.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

// ============================================================
// file subcommands
// ============================================================
TEST(FileTest, Dirname) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "file dirname /a/b/c.txt"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "/a/b");
    Tcl_DeleteInterp(i);
}
TEST(FileTest, Tail) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "file tail /a/b/c.txt"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "c.txt");
    Tcl_DeleteInterp(i);
}
TEST(FileTest, Extension) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "file extension test.tcl"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), ".tcl");
    Tcl_DeleteInterp(i);
}
TEST(FileTest, Join) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "file join /a b c.txt"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "/a/b/c.txt");
    Tcl_DeleteInterp(i);
}
TEST(FileTest, Nativename) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "file nativename /a/b/c"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "/a/b/c");
    Tcl_DeleteInterp(i);
}
TEST(FileTest, ExistsTrue) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "file exists /"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}
TEST(FileTest, ExistsFalse) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "file exists /nonexistent_path_xyz"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "0");
    Tcl_DeleteInterp(i);
}
TEST(FileTest, IsDirectory) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "file isdirectory /tmp"), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "1");
    Tcl_DeleteInterp(i);
}

// ============================================================
// source and Tcl_EvalFile
// ============================================================
TEST(FileTest, SourceFile) {
    // Create a temp file
    auto tmpPath = std::filesystem::temp_directory_path() / "minitcl_test.tcl";
    {
        std::ofstream f(tmpPath);
        f << "set x 42\nset y hello\n";
    }
    Tcl_Interp* i = Tcl_CreateInterp();
    std::string cmd = "source " + tmpPath.string();
    ASSERT_EQ(Tcl_Eval(i, cmd.c_str()), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "x", 0), "42");
    EXPECT_STREQ(Tcl_GetVar(i, "y", 0), "hello");
    Tcl_DeleteInterp(i);
    std::filesystem::remove(tmpPath);
}
TEST(FileTest, SourceNonexistent) {
    Tcl_Interp* i = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(i, "source /nonexistent_file.tcl"), TCL_ERROR);
    Tcl_DeleteInterp(i);
}
TEST(FileTest, EvalFile) {
    auto tmpPath = std::filesystem::temp_directory_path() / "minitcl_evalfile.tcl";
    {
        std::ofstream f(tmpPath);
        f << "set result 99\n";
    }
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_EvalFile(i, tmpPath.c_str()), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "result", 0), "99");
    Tcl_DeleteInterp(i);
    std::filesystem::remove(tmpPath);
}

// ============================================================
// open, close, gets, read, eof
// ============================================================
TEST(IOTest, OpenReadClose) {
    auto tmpPath = std::filesystem::temp_directory_path() / "minitcl_io_test.txt";
    {
        std::ofstream f(tmpPath);
        f << "line1\nline2\n";
    }
    Tcl_Interp* i = Tcl_CreateInterp();
    std::string cmd = "open " + tmpPath.string() + " r";
    ASSERT_EQ(Tcl_Eval(i, cmd.c_str()), TCL_OK);
    std::string chanName = Tcl_GetStringResult(i);

    // gets
    cmd = "gets " + chanName + " line";
    ASSERT_EQ(Tcl_Eval(i, cmd.c_str()), TCL_OK);
    EXPECT_STREQ(Tcl_GetVar(i, "line", 0), "line1");

    // read rest
    cmd = "read " + chanName;
    ASSERT_EQ(Tcl_Eval(i, cmd.c_str()), TCL_OK);
    EXPECT_STREQ(Tcl_GetStringResult(i), "line2\n");

    // close
    cmd = "close " + chanName;
    ASSERT_EQ(Tcl_Eval(i, cmd.c_str()), TCL_OK);

    Tcl_DeleteInterp(i);
    std::filesystem::remove(tmpPath);
}
TEST(IOTest, OpenNonexistent) {
    Tcl_Interp* i = Tcl_CreateInterp();
    EXPECT_EQ(Tcl_Eval(i, "open /nonexistent_xyz r"), TCL_ERROR);
    Tcl_DeleteInterp(i);
}
TEST(IOTest, FlushStdout) {
    Tcl_Interp* i = Tcl_CreateInterp();
    ASSERT_EQ(Tcl_Eval(i, "flush stdout"), TCL_OK);
    Tcl_DeleteInterp(i);
}
