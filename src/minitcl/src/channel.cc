// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Channel system for ReportTcl.cc compatibility
//
// ReportTcl.cc needs:
// - Tcl_GetStdChannel(TCL_STDOUT/TCL_STDERR)
// - Tcl_StackChannel(interp, typePtr, clientData, mask, prevChan)
// - Tcl_UnstackChannel(interp, chan)
// - Tcl_GetChannelType(chan)
// - Tcl_GetChannelInstanceData(chan)
// - Tcl_ChannelOutputProc(typePtr)
// - Tcl_Flush(chan)

#include "channel.h"

#include <cstdio>
#include <mutex>
#include <vector>

namespace minitcl {

struct ChannelImpl {
    const Tcl_ChannelType *type;
    ClientData instanceData;
    int mask;
    ChannelImpl *parent;  // for stacking
    FILE *fp;             // for standard channels
};

// Pre-allocated standard channel output proc
static int stdOutputProc(ClientData instanceData, const char *buf,
                          int toWrite, int *errorCodePtr) {
    auto *ch = static_cast<ChannelImpl *>(instanceData);
    if (ch && ch->fp) {
        size_t written = fwrite(buf, 1, toWrite, ch->fp);
        if (written < (size_t)toWrite) {
            if (errorCodePtr) *errorCodePtr = errno;
            return -1;
        }
    }
    return toWrite;
}

static Tcl_ChannelType stdChannelType = {
    "file",
    TCL_CHANNEL_VERSION_5,
    nullptr,       // closeProc (unused in Tcl 9)
    nullptr,       // inputProc
    reinterpret_cast<Tcl_DriverOutputProc *>(stdOutputProc),
    nullptr,       // close2Proc
    nullptr,       // setOptionProc
    nullptr,       // getOptionProc
    nullptr,       // watchProc
    nullptr,       // getHandleProc
    nullptr,       // close2Proc2
    nullptr,       // blockModeProc
    nullptr,       // flushProc
    nullptr,       // handlerProc
    nullptr,       // wideSeekProc
    nullptr,       // threadActionProc
    nullptr        // truncateProc
};

static ChannelImpl stdoutChannel = {&stdChannelType, nullptr, TCL_WRITABLE, nullptr, stdout};
static ChannelImpl stderrChannel = {&stdChannelType, nullptr, TCL_WRITABLE, nullptr, stderr};
static ChannelImpl stdinChannel  = {&stdChannelType, nullptr, TCL_READABLE, nullptr, stdin};

// Track all stacked channels for cleanup
static std::vector<ChannelImpl *> stackedChannels;

static std::once_flag channelsInitFlag;

static void doInitChannels() {
    stdoutChannel.instanceData = &stdoutChannel;
    stderrChannel.instanceData = &stderrChannel;
    stdinChannel.instanceData = &stdinChannel;
}

void initChannels() {
    std::call_once(channelsInitFlag, doInitChannels);
}

}  // namespace minitcl

// ============================================================
// Channel C API implementation
// ============================================================

Tcl_Channel Tcl_GetStdChannel(int type) {
    minitcl::initChannels();
    if (type == TCL_STDOUT) return &minitcl::stdoutChannel;
    if (type == TCL_STDERR) return &minitcl::stderrChannel;
    if (type == TCL_STDIN) return &minitcl::stdinChannel;
    return nullptr;
}

Tcl_Channel Tcl_StackChannel(Tcl_Interp *, const Tcl_ChannelType *typePtr,
                              ClientData instanceData, int mask,
                              Tcl_Channel prevChan) {
    auto *prev = static_cast<minitcl::ChannelImpl *>(prevChan);
    auto *stacked = new minitcl::ChannelImpl();
    stacked->type = typePtr;
    stacked->instanceData = instanceData;
    stacked->mask = mask;
    stacked->parent = prev;
    stacked->fp = prev ? prev->fp : nullptr;
    minitcl::stackedChannels.push_back(stacked);
    return stacked;
}

int Tcl_UnstackChannel(Tcl_Interp *, Tcl_Channel chan) {
    auto *ch = static_cast<minitcl::ChannelImpl *>(chan);
    // Remove from tracking
    auto &v = minitcl::stackedChannels;
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (*it == ch) { v.erase(it); break; }
    }
    delete ch;
    return TCL_OK;
}

int Tcl_Flush(Tcl_Channel chan) {
    auto *ch = static_cast<minitcl::ChannelImpl *>(chan);
    if (ch && ch->fp) fflush(ch->fp);
    return TCL_OK;
}

const Tcl_ChannelType *Tcl_GetChannelType(Tcl_Channel chan) {
    auto *ch = static_cast<minitcl::ChannelImpl *>(chan);
    return ch ? ch->type : nullptr;
}

ClientData Tcl_GetChannelInstanceData(Tcl_Channel chan) {
    auto *ch = static_cast<minitcl::ChannelImpl *>(chan);
    return ch ? ch->instanceData : nullptr;
}

Tcl_DriverOutputProc *Tcl_ChannelOutputProc(const Tcl_ChannelType *typePtr) {
    return typePtr ? typePtr->outputProc : nullptr;
}
