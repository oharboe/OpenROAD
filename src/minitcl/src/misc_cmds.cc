// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Miscellaneous commands (clock, exec, package, encoding, etc.)

#include "misc_cmds.h"
#include "interp.h"
#include "string_cmds.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <unistd.h>

namespace minitcl {

// ============================================================
// clock command
// ============================================================

static int clockCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }
    const char *sub = Tcl_GetString(objv[1]);

    if (strcmp(sub, "seconds") == 0) {
        auto now = std::chrono::system_clock::now();
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                         now.time_since_epoch())
                         .count();
        Tcl_SetObjResult(interp, Tcl_NewWideIntObj(secs));
        return TCL_OK;
    }

    if (strcmp(sub, "milliseconds") == 0) {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch())
                       .count();
        Tcl_SetObjResult(interp, Tcl_NewWideIntObj(ms));
        return TCL_OK;
    }

    if (strcmp(sub, "format") == 0) {
        if (objc < 3) {
            getImpl(interp)->result = "wrong # args";
            return TCL_ERROR;
        }
        long long clockVal = 0;
        if (Tcl_GetWideIntFromObj(interp, objv[2], &clockVal) != TCL_OK)
            return TCL_ERROR;

        const char *fmt = "%a %b %d %H:%M:%S %Z %Y";  // default
        for (int i = 3; i + 1 < objc; i += 2) {
            if (strcmp(Tcl_GetString(objv[i]), "-format") == 0) {
                fmt = Tcl_GetString(objv[i + 1]);
            }
        }

        time_t t = static_cast<time_t>(clockVal);
        struct tm *tm = localtime(&t);
        char buf[256];
        strftime(buf, sizeof(buf), fmt, tm);
        Tcl_SetObjResult(interp, Tcl_NewStringObj(buf, -1));
        return TCL_OK;
    }

    if (strcmp(sub, "scan") == 0) {
        // Simplified stub
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }

    getImpl(interp)->result = std::string("unknown clock subcommand \"") + sub + "\"";
    return TCL_ERROR;
}

// ============================================================
// exec command
// ============================================================

static int execCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }

    // Build command string
    std::string cmd;
    int startIdx = 1;
    // Skip optional flags
    while (startIdx < objc) {
        const char *arg = Tcl_GetString(objv[startIdx]);
        if (arg[0] == '-' && strcmp(arg, "--") != 0) {
            startIdx++;
        } else {
            if (strcmp(arg, "--") == 0) startIdx++;
            break;
        }
    }

    for (int i = startIdx; i < objc; i++) {
        if (i > startIdx) cmd += ' ';
        cmd += Tcl_GetString(objv[i]);
    }

    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) {
        getImpl(interp)->result = "couldn't execute \"" + cmd + "\"";
        return TCL_ERROR;
    }

    std::string output;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp)) output += buf;
    int status = pclose(fp);

    // Remove trailing newline
    if (!output.empty() && output.back() == '\n') output.pop_back();

    Tcl_SetObjResult(interp, Tcl_NewStringObj(output.c_str(), output.size()));

    if (status != 0) {
        // Non-zero exit is an error in Tcl exec
        return TCL_ERROR;
    }
    return TCL_OK;
}

// ============================================================
// package command (stubs)
// ============================================================

static int packageCmd(ClientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }
    const char *sub = Tcl_GetString(objv[1]);

    if (strcmp(sub, "require") == 0) {
        // No-op: all code is statically linked
        Tcl_SetObjResult(interp, Tcl_NewStringObj("0.0", -1));
        return TCL_OK;
    }
    if (strcmp(sub, "provide") == 0) {
        return TCL_OK;
    }
    if (strcmp(sub, "ifneeded") == 0) {
        return TCL_OK;
    }
    if (strcmp(sub, "names") == 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("", 0));
        return TCL_OK;
    }

    return TCL_OK;
}

// ============================================================
// encoding command (stub)
// ============================================================

static int encodingCmd(ClientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *const objv[]) {
    if (objc >= 2 && strcmp(Tcl_GetString(objv[1]), "system") == 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("utf-8", -1));
        return TCL_OK;
    }
    return TCL_OK;
}

// ============================================================
// history command (stub)
// ============================================================

static int historyCmd(ClientData, Tcl_Interp *, int, Tcl_Obj *const[]) {
    return TCL_OK;
}

// ============================================================
// after command (stub)
// ============================================================

static int afterCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc >= 3 && strcmp(Tcl_GetString(objv[1]), "idle") == 0) {
        // Execute the script immediately (no event loop)
        return Tcl_Eval(interp, Tcl_GetString(objv[2]));
    }
    return TCL_OK;
}

// ============================================================
// update command (stub)
// ============================================================

static int updateCmd(ClientData, Tcl_Interp *, int, Tcl_Obj *const[]) {
    return TCL_OK;
}

// ============================================================
// lmap command
// ============================================================

static int lmapCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc != 4) {
        getImpl(interp)->result = "wrong # args: should be \"lmap varName list body\"";
        return TCL_ERROR;
    }
    const char *varName = Tcl_GetString(objv[1]);
    auto elems = parseList(Tcl_GetString(objv[2]));
    const char *body = Tcl_GetString(objv[3]);

    std::vector<std::string> results;
    for (const auto &elem : elems) {
        Tcl_SetVar(interp, varName, elem.c_str(), 0);
        int code = Tcl_Eval(interp, body);
        if (code == TCL_BREAK) break;
        if (code == TCL_CONTINUE) continue;
        if (code != TCL_OK) return code;
        results.push_back(Tcl_GetStringResult(interp));
    }

    // Build result list
    std::string result;
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) result += ' ';
        auto &s = results[i];
        bool needsQ = s.empty();
        for (char c : s)
            if (c == ' ' || c == '\t' || c == '\n') { needsQ = true; break; }
        if (needsQ) { result += '{'; result += s; result += '}'; }
        else result += s;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
    return TCL_OK;
}

// ============================================================
// try command
// ============================================================

static int tryCmd(ClientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }

    int code = Tcl_Eval(interp, Tcl_GetString(objv[1]));
    std::string savedResult = Tcl_GetStringResult(interp);

    // Look for "finally" clause
    for (int i = 2; i < objc; i++) {
        if (strcmp(Tcl_GetString(objv[i]), "finally") == 0 && i + 1 < objc) {
            Tcl_Eval(interp, Tcl_GetString(objv[i + 1]));
            break;
        }
        if (strcmp(Tcl_GetString(objv[i]), "on") == 0 && i + 3 < objc) {
            // try body on code varList body
            int matchCode = atoi(Tcl_GetString(objv[i + 1]));
            if (code == matchCode) {
                const char *handlerBody = Tcl_GetString(objv[i + 3]);
                code = Tcl_Eval(interp, handlerBody);
                return code;
            }
            i += 3;
        }
    }

    // Restore original result (clear resultObj to avoid stale object
    // from finally block taking priority in Tcl_GetStringResult)
    if (code != TCL_OK) {
        auto *impl = getImpl(interp);
        if (impl->resultObj) {
            Tcl_DecrRefCount(impl->resultObj);
            impl->resultObj = nullptr;
        }
        impl->result = savedResult;
    }
    return code;
}

// ============================================================
// pid command (stub)
// ============================================================

static int pidCmd(ClientData, Tcl_Interp *interp, int, Tcl_Obj *const[]) {
    Tcl_SetObjResult(interp, Tcl_NewIntObj(getpid()));
    return TCL_OK;
}

// ============================================================
// pwd command
// ============================================================

static int pwdCmd(ClientData, Tcl_Interp *interp, int, Tcl_Obj *const[]) {
    char buf[4096];
    if (getcwd(buf, sizeof(buf))) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(buf, -1));
    }
    return TCL_OK;
}

// ============================================================
// cd command
// ============================================================

static int cdCmd(ClientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const objv[]) {
    const char *dir = objc >= 2 ? Tcl_GetString(objv[1]) : getenv("HOME");
    if (dir && chdir(dir) != 0) {
        getImpl(interp)->result = std::string("couldn't change directory to \"") + dir + "\"";
        return TCL_ERROR;
    }
    return TCL_OK;
}

// ============================================================
// exit command
// ============================================================

static int exitCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    int code = 0;
    if (objc >= 2) {
        if (Tcl_GetIntFromObj(interp, objv[1], &code) != TCL_OK)
            return TCL_ERROR;
    }
    std::exit(code);
    return TCL_OK;  // unreachable
}

// ============================================================
// trace command (stub - variable tracing not implemented)
// ============================================================

static int traceCmd(ClientData, Tcl_Interp *, int, Tcl_Obj *const[]) {
    // No-op stub: variable tracing is not implemented in minitcl.
    // STA's Variables.tcl uses "trace variable" to sync Tcl vars
    // with C++ state; the C++ side handles this directly.
    return TCL_OK;
}

// ============================================================
// interp command (partial - supports alias subcommand)
// ============================================================

static int interpCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }
    const char *sub = Tcl_GetString(objv[1]);
    if (strcmp(sub, "alias") == 0) {
        // interp alias srcPath srcCmd targetPath targetCmd ?args...?
        // e.g.: interp alias {} replace_design {} replace_hier_module
        if (objc < 6) {
            getImpl(interp)->result = "wrong # args: should be \"interp alias srcPath srcCmd targetPath targetCmd ?arg ...?\"";
            return TCL_ERROR;
        }
        // objv[2] = srcPath (ignored, {} = current interp)
        const char *newCmd = Tcl_GetString(objv[3]);
        // objv[4] = targetPath (ignored)
        const char *targetCmd = Tcl_GetString(objv[5]);
        // Create alias by registering a command that evals the target
        auto *impl = getImpl(interp);
        auto it = impl->commands.find(targetCmd);
        if (it == impl->commands.end()) {
            // Try namespace-qualified
            std::string qualified = std::string("sta::") + targetCmd;
            it = impl->commands.find(qualified);
            if (it != impl->commands.end()) {
                auto imported = it->second;
                imported.deleteProc = nullptr;
                impl->commands[newCmd] = imported;
                return TCL_OK;
            }
            impl->result = std::string("invalid command name \"") + targetCmd + "\"";
            return TCL_ERROR;
        }
        auto imported = it->second;
        imported.deleteProc = nullptr;
        impl->commands[newCmd] = imported;
        return TCL_OK;
    }
    // Other subcommands are no-ops
    return TCL_OK;
}

// ============================================================
// auto_path variable and unknown handler
// ============================================================

static int unknownHandler(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *const objv[]) {
    if (objc < 2) return TCL_ERROR;
    getImpl(interp)->result = std::string("invalid command name \"") +
                               Tcl_GetString(objv[1]) + "\"";
    return TCL_ERROR;
}

void registerMiscCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "clock", clockCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "exec", execCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "package", packageCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "encoding", encodingCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "history", historyCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "after", afterCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "update", updateCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "lmap", lmapCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "try", tryCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "pid", pidCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "pwd", pwdCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "cd", cdCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "exit", exitCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "trace", traceCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "interp", interpCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "unknown", unknownHandler, nullptr, nullptr);

    // Set auto_path variable
    Tcl_SetVar(interp, "auto_path", "", TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "tcl_platform(platform)", "unix", TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "tcl_platform(os)", "Linux", TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "tcl_version", "9.0", TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "errorInfo", "", TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "errorCode", "NONE", TCL_GLOBAL_ONLY);
}

}  // namespace minitcl
