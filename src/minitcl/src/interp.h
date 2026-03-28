// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Interpreter internals

#pragma once

#include <map>
#include <string>
#include <vector>

#include "tcl.h"

namespace minitcl {

// Internal interpreter state
struct InterpImpl {
    // Result
    std::string result;
    Tcl_Obj *resultObj = nullptr;

    // Associated data
    std::map<std::string, std::pair<Tcl_InterpDeleteProc *, ClientData>>
        assocData;

    // Command registry
    struct CmdEntry {
        Tcl_ObjCmdProc *objProc = nullptr;
        Tcl_CmdProc *stringProc = nullptr;
        ClientData clientData = nullptr;
        Tcl_CmdDeleteProc *deleteProc = nullptr;
    };
    std::map<std::string, CmdEntry> commands;

    // Variables (simple flat map for now, extended in Phase 4)
    std::map<std::string, std::string> variables;

    bool deleted = false;
};

InterpImpl *getImpl(Tcl_Interp *interp);

}  // namespace minitcl
