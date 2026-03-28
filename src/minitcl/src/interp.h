// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Interpreter internals

#pragma once

#include <map>
#include <string>
#include <vector>

#include "tcl.h"

namespace minitcl {

// A call frame holds local variables for a proc invocation
struct CallFrame {
    std::map<std::string, std::string> locals;
    // Links for upvar: local_name -> {level, target_name}
    std::map<std::string, std::pair<int, std::string>> upvarLinks;
};

// Proc definition
struct ProcDef {
    std::vector<std::string> params;           // parameter names
    std::vector<std::string> defaults;         // default values ("" = no default)
    std::vector<bool> hasDefault;              // whether param has default
    bool hasArgs = false;                       // last param is "args"
    std::string body;
};

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

    // Global variables
    std::map<std::string, std::string> globals;

    // Call frame stack (empty = global scope)
    std::vector<CallFrame> callStack;

    // Proc definitions
    std::map<std::string, ProcDef> procs;

    bool deleted = false;

    // Get/set variable respecting scope
    const char *getVar(const std::string &name) const;
    void setVar(const std::string &name, const std::string &value);
    bool unsetVar(const std::string &name);
    bool varExists(const std::string &name) const;

    // Access variable in a specific frame
    const char *getVarInFrame(int frameIdx, const std::string &name) const;
    void setVarInFrame(int frameIdx, const std::string &name,
                        const std::string &value);
};

InterpImpl *getImpl(Tcl_Interp *interp);

}  // namespace minitcl
