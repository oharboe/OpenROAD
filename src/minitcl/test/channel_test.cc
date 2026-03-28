// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 13 - Channel system tests (ReportTcl.cc compatibility)

#include "tcl.h"
#include <cstring>
#include <gtest/gtest.h>

TEST(ChannelTest, GetStdChannelStdout) {
    Tcl_Channel ch = Tcl_GetStdChannel(TCL_STDOUT);
    ASSERT_NE(ch, nullptr);
}

TEST(ChannelTest, GetStdChannelStderr) {
    Tcl_Channel ch = Tcl_GetStdChannel(TCL_STDERR);
    ASSERT_NE(ch, nullptr);
}

TEST(ChannelTest, GetChannelType) {
    Tcl_Channel ch = Tcl_GetStdChannel(TCL_STDOUT);
    const Tcl_ChannelType *type = Tcl_GetChannelType(ch);
    ASSERT_NE(type, nullptr);
    EXPECT_STREQ(type->typeName, "file");
}

TEST(ChannelTest, GetChannelInstanceData) {
    Tcl_Channel ch = Tcl_GetStdChannel(TCL_STDOUT);
    ClientData data = Tcl_GetChannelInstanceData(ch);
    EXPECT_NE(data, nullptr);
}

TEST(ChannelTest, ChannelOutputProc) {
    Tcl_Channel ch = Tcl_GetStdChannel(TCL_STDOUT);
    const Tcl_ChannelType *type = Tcl_GetChannelType(ch);
    Tcl_DriverOutputProc *proc = Tcl_ChannelOutputProc(type);
    EXPECT_NE(proc, nullptr);
}

TEST(ChannelTest, FlushStdout) {
    Tcl_Channel ch = Tcl_GetStdChannel(TCL_STDOUT);
    EXPECT_EQ(Tcl_Flush(ch), TCL_OK);
}

// Test the stacking pattern used by ReportTcl.cc
TEST(ChannelTest, StackAndUnstack) {
    Tcl_Interp *interp = Tcl_CreateInterp();
    Tcl_Channel stdout_ch = Tcl_GetStdChannel(TCL_STDOUT);

    // Create a custom channel type (like ReportTcl does)
    static int callCount = 0;
    static Tcl_ChannelType customType = {
        "custom",
        TCL_CHANNEL_VERSION_5,
        nullptr,  // closeProc
        nullptr,  // inputProc
        [](ClientData cd, const char *buf, int toWrite, int *) -> int {
            callCount++;
            (void)cd; (void)buf;
            return toWrite;
        },
        nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    };

    int myData = 42;
    Tcl_Channel stacked = Tcl_StackChannel(interp, &customType, &myData,
                                            TCL_WRITABLE, stdout_ch);
    ASSERT_NE(stacked, nullptr);

    // Verify we can get back the type and instance data
    EXPECT_EQ(Tcl_GetChannelType(stacked), &customType);
    EXPECT_EQ(Tcl_GetChannelInstanceData(stacked), &myData);

    // Get the output proc
    Tcl_DriverOutputProc *proc = Tcl_ChannelOutputProc(&customType);
    ASSERT_NE(proc, nullptr);

    // Call it
    int errCode = 0;
    callCount = 0;
    proc(&myData, "test", 4, &errCode);
    EXPECT_EQ(callCount, 1);

    // Unstack
    EXPECT_EQ(Tcl_UnstackChannel(interp, stacked), TCL_OK);

    Tcl_DeleteInterp(interp);
}

TEST(ChannelTest, NullChannelOutputProc) {
    EXPECT_EQ(Tcl_ChannelOutputProc(nullptr), nullptr);
}
