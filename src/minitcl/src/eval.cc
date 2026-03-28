// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Evaluation engine

#include "eval.h"
#include "interp.h"
#include "parser.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

namespace minitcl {

// Find matching close bracket, respecting nesting and quoting
static const char *findCloseBracket(const char *p) {
    int depth = 1;
    while (*p && depth > 0) {
        if (*p == '[') {
            depth++;
        } else if (*p == ']') {
            depth--;
            if (depth == 0) return p;
        } else if (*p == '{') {
            // Skip braced content (no substitution inside)
            int braceDepth = 1;
            p++;
            while (*p && braceDepth > 0) {
                if (*p == '\\' && *(p + 1)) { p += 2; continue; }
                if (*p == '{') braceDepth++;
                else if (*p == '}') braceDepth--;
                p++;
            }
            continue;
        } else if (*p == '"') {
            // Skip quoted content
            p++;
            while (*p && *p != '"') {
                if (*p == '\\' && *(p + 1)) p++;
                p++;
            }
            if (*p == '"') p++;
            continue;
        } else if (*p == '\\' && *(p + 1)) {
            p++;  // skip escaped char
        }
        p++;
    }
    return nullptr;
}

// Parse a variable name starting at position after $
// Returns the variable name and advances pos past it
static std::string parseVarName(const char *&p) {
    std::string name;

    if (*p == '{') {
        // ${varname} - braced variable name
        p++;
        while (*p && *p != '}') {
            name += *p++;
        }
        if (*p == '}') p++;
        return name;
    }

    // Regular variable name: alphanumeric, underscore, ::
    while (*p) {
        if (*p == ':' && *(p + 1) == ':') {
            name += "::";
            p += 2;
        } else if (isalnum(*p) || *p == '_') {
            name += *p++;
        } else {
            break;
        }
    }

    // Check for array index: var(index)
    if (*p == '(') {
        name += *p++;
        // Read until matching )
        int depth = 1;
        while (*p && depth > 0) {
            if (*p == '(') depth++;
            else if (*p == ')') {
                depth--;
                if (depth == 0) { p++; break; }
            }
            name += *p++;
        }
        name += ')';  // closing paren was consumed but not added
        // Actually let me fix this - we consumed ')' with p++ but didn't add it
        // The name should include everything including ()
    }

    return name;
}

std::string substitute(Tcl_Interp *interp, const std::string &str, int *code) {
    std::string result;
    result.reserve(str.size());
    *code = TCL_OK;

    const char *p = str.c_str();
    while (*p) {
        if (*p == '$') {
            p++;  // skip $
            std::string varName = parseVarName(p);
            if (varName.empty()) {
                result += '$';
                continue;
            }
            const char *val = Tcl_GetVar(interp, varName.c_str(), 0);
            if (!val) {
                auto *impl = getImpl(interp);
                impl->result = "can't read \"" + varName + "\": no such variable";
                *code = TCL_ERROR;
                return "";
            }
            result += val;
        } else if (*p == '[') {
            p++;  // skip [
            const char *end = findCloseBracket(p);
            if (!end) {
                auto *impl = getImpl(interp);
                impl->result = "missing close-bracket";
                *code = TCL_ERROR;
                return "";
            }
            std::string cmdStr(p, end - p);
            int rc = Tcl_Eval(interp, cmdStr.c_str());
            if (rc != TCL_OK) {
                *code = rc;
                return "";
            }
            result += Tcl_GetStringResult(interp);
            p = end + 1;  // skip ]
        } else if (*p == '\\') {
            // Backslash substitution
            std::string esc;
            esc += *p++;
            if (*p) esc += *p++;
            result += backslashSubst(esc);
        } else {
            result += *p++;
        }
    }

    return result;
}

int evalCommand(Tcl_Interp *interp, const std::vector<std::string> &words) {
    if (words.empty()) return TCL_OK;

    auto *impl = getImpl(interp);
    Tcl_ResetResult(interp);

    const std::string &cmdName = words[0];

    // Look up command
    auto it = impl->commands.find(cmdName);
    if (it == impl->commands.end()) {
        impl->result = "invalid command name \"" + cmdName + "\"";
        return TCL_ERROR;
    }

    auto &cmd = it->second;

    if (cmd.objProc) {
        // Obj-based command
        std::vector<Tcl_Obj *> objv;
        objv.reserve(words.size());
        for (const auto &w : words) {
            Tcl_Obj *obj = Tcl_NewStringObj(w.c_str(), w.size());
            Tcl_IncrRefCount(obj);
            objv.push_back(obj);
        }

        int rc = cmd.objProc(cmd.clientData, interp,
                              static_cast<int>(objv.size()), objv.data());

        for (auto *obj : objv) {
            Tcl_DecrRefCount(obj);
        }
        return rc;
    } else if (cmd.stringProc) {
        // String-based command
        std::vector<const char *> argv;
        argv.reserve(words.size());
        for (const auto &w : words) {
            argv.push_back(w.c_str());
        }

        return cmd.stringProc(cmd.clientData, interp,
                               static_cast<int>(argv.size()), argv.data());
    }

    impl->result = "invalid command name \"" + cmdName + "\"";
    return TCL_ERROR;
}

// ============================================================
// Built-in commands
// ============================================================

static int setCmd(ClientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const objv[]) {
    if (objc < 2 || objc > 3) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"set varName ?newValue?\"";
        return TCL_ERROR;
    }

    const char *varName = Tcl_GetString(objv[1]);

    if (objc == 3) {
        const char *value = Tcl_GetString(objv[2]);
        Tcl_SetVar(interp, varName, value, 0);
    }

    const char *val = Tcl_GetVar(interp, varName, 0);
    if (!val) {
        auto *impl = getImpl(interp);
        impl->result = std::string("can't read \"") + varName + "\": no such variable";
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(val, -1));
    return TCL_OK;
}

static int unsetCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    auto *impl = getImpl(interp);
    for (int i = 1; i < objc; i++) {
        const char *varName = Tcl_GetString(objv[i]);
        impl->variables.erase(varName);
    }
    return TCL_OK;
}

static int putsCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2 || objc > 4) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"puts ?-nonewline? ?channelId? string\"";
        return TCL_ERROR;
    }

    bool nonewline = false;
    int strIdx = 1;
    FILE *channel = stdout;

    if (objc >= 3 && strcmp(Tcl_GetString(objv[1]), "-nonewline") == 0) {
        nonewline = true;
        strIdx = 2;
    }

    // Check for channel argument
    if (strIdx < objc - 1) {
        const char *chanName = Tcl_GetString(objv[strIdx]);
        if (strcmp(chanName, "stderr") == 0) {
            channel = stderr;
        } else if (strcmp(chanName, "stdout") == 0) {
            channel = stdout;
        }
        strIdx++;
    }

    if (strIdx >= objc) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"puts ?-nonewline? ?channelId? string\"";
        return TCL_ERROR;
    }

    const char *str = Tcl_GetString(objv[strIdx]);
    fputs(str, channel);
    if (!nonewline) fputc('\n', channel);
    fflush(channel);
    return TCL_OK;
}

void registerBuiltins(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "set", setCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "unset", unsetCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "puts", putsCmd, nullptr, nullptr);
}

}  // namespace minitcl
