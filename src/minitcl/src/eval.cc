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
        impl->unsetVar(varName);
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

static int procCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc != 4) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"proc name args body\"";
        return TCL_ERROR;
    }

    auto *impl = getImpl(interp);
    const char *name = Tcl_GetString(objv[1]);
    const char *argsStr = Tcl_GetString(objv[2]);
    const char *body = Tcl_GetString(objv[3]);

    ProcDef proc;
    proc.body = body;

    // Parse parameter list
    auto paramWords = parseScript(argsStr);
    // The param list is a Tcl list, so parse it as words
    // Actually, we need to parse it as a list. For now, simple split on space
    // respecting braces
    for (const auto &cmd : paramWords) {
        for (const auto &word : cmd.words) {
            const std::string &param = word.text;
            // Check if this is a {name default} pair
            auto inner = parseScript(param.c_str());
            if (!inner.empty() && inner[0].words.size() == 2) {
                proc.params.push_back(inner[0].words[0].text);
                proc.defaults.push_back(inner[0].words[1].text);
                proc.hasDefault.push_back(true);
            } else {
                if (param == "args") {
                    proc.hasArgs = true;
                    proc.params.push_back("args");
                    proc.defaults.push_back("");
                    proc.hasDefault.push_back(false);
                } else {
                    proc.params.push_back(param);
                    proc.defaults.push_back("");
                    proc.hasDefault.push_back(false);
                }
            }
        }
    }

    impl->procs[name] = std::move(proc);

    // Register a command that invokes the proc
    std::string *procName = new std::string(name);
    Tcl_CreateObjCommand(
        interp, name,
        [](ClientData cd, Tcl_Interp *interp, int objc,
           Tcl_Obj *const objv[]) -> int {
            auto *pName = static_cast<std::string *>(cd);
            auto *impl = getImpl(interp);
            auto it = impl->procs.find(*pName);
            if (it == impl->procs.end()) {
                impl->result = "proc not found: " + *pName;
                return TCL_ERROR;
            }

            const auto &proc = it->second;
            CallFrame frame;

            // Bind arguments to parameters
            int argIdx = 1;
            for (size_t i = 0; i < proc.params.size(); i++) {
                if (proc.params[i] == "args" && proc.hasArgs &&
                    i == proc.params.size() - 1) {
                    // Collect remaining args into a list
                    std::string argsList;
                    for (int j = argIdx; j < objc; j++) {
                        if (j > argIdx) argsList += ' ';
                        const char *s = Tcl_GetString(objv[j]);
                        // Simple quoting
                        bool needsQuoting = false;
                        for (const char *p = s; *p; p++) {
                            if (*p == ' ' || *p == '\t' || *p == '\n') {
                                needsQuoting = true;
                                break;
                            }
                        }
                        if (needsQuoting) {
                            argsList += '{';
                            argsList += s;
                            argsList += '}';
                        } else {
                            argsList += s;
                        }
                    }
                    frame.locals["args"] = argsList;
                    argIdx = objc;
                } else if (argIdx < objc) {
                    frame.locals[proc.params[i]] = Tcl_GetString(objv[argIdx++]);
                } else if (proc.hasDefault[i]) {
                    frame.locals[proc.params[i]] = proc.defaults[i];
                } else {
                    impl->result = "wrong # args: should be \"" + *pName;
                    for (const auto &p : proc.params) {
                        impl->result += " " + p;
                    }
                    impl->result += "\"";
                    return TCL_ERROR;
                }
            }

            // Check for too many args (unless proc has "args")
            if (argIdx < objc && !proc.hasArgs) {
                impl->result = "wrong # args: should be \"" + *pName;
                for (const auto &p : proc.params) {
                    impl->result += " " + p;
                }
                impl->result += "\"";
                return TCL_ERROR;
            }

            // Push frame and evaluate body
            impl->callStack.push_back(std::move(frame));
            int code = Tcl_Eval(interp, proc.body.c_str());
            impl->callStack.pop_back();

            // TCL_RETURN becomes TCL_OK at proc boundary
            if (code == TCL_RETURN) code = TCL_OK;

            return code;
        },
        procName,
        [](ClientData cd) { delete static_cast<std::string *>(cd); });

    return TCL_OK;
}

static int returnCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc > 2) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"return ?result?\"";
        return TCL_ERROR;
    }
    if (objc == 2) {
        Tcl_SetObjResult(interp, objv[1]);
    }
    return TCL_RETURN;
}

static int errorCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc < 2) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"error message ?info? ?code?\"";
        return TCL_ERROR;
    }
    auto *impl = getImpl(interp);
    impl->result = Tcl_GetString(objv[1]);
    return TCL_ERROR;
}

static int globalCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    auto *impl = getImpl(interp);
    if (impl->callStack.empty()) return TCL_OK;  // Already global

    auto &frame = impl->callStack.back();
    for (int i = 1; i < objc; i++) {
        const char *varName = Tcl_GetString(objv[i]);
        // Create upvar link to global scope (-1 = global)
        frame.upvarLinks[varName] = {-1, varName};
    }
    return TCL_OK;
}

void registerBuiltins(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "set", setCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "unset", unsetCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "puts", putsCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "proc", procCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "return", returnCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "error", errorCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "global", globalCmd, nullptr, nullptr);
}

}  // namespace minitcl
