// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Dict, array, info, glob commands

#include "dict_array.h"
#include "interp.h"
#include "string_cmds.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <unistd.h>
#include <string>
#include <vector>

namespace minitcl {

// ============================================================
// dict command
// ============================================================

static int dictCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }
    const char *sub = Tcl_GetString(objv[1]);

    if (strcmp(sub, "create") == 0) {
        // dict create key val key val ...
        std::string result;
        for (int i = 2; i < objc; i++) {
            if (i > 2) result += ' ';
            const char *s = Tcl_GetString(objv[i]);
            bool needsQ = false;
            for (const char *p = s; *p; p++)
                if (*p == ' ' || *p == '\t') { needsQ = true; break; }
            if (needsQ || *s == '\0') { result += '{'; result += s; result += '}'; }
            else result += s;
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "get") == 0) {
        if (objc < 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto elems = parseList(Tcl_GetString(objv[2]));
        const char *key = Tcl_GetString(objv[3]);
        for (size_t i = 0; i + 1 < elems.size(); i += 2) {
            if (elems[i] == key) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj(elems[i + 1].c_str(), -1));
                return TCL_OK;
            }
        }
        getImpl(interp)->result = std::string("key \"") + key + "\" not known in dictionary";
        return TCL_ERROR;
    }

    if (strcmp(sub, "set") == 0) {
        if (objc != 5) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        const char *varName = Tcl_GetString(objv[2]);
        const char *key = Tcl_GetString(objv[3]);
        const char *val = Tcl_GetString(objv[4]);
        const char *cur = Tcl_GetVar(interp, varName, 0);
        std::vector<std::string> elems;
        if (cur && *cur) elems = parseList(cur);
        bool found = false;
        for (size_t i = 0; i + 1 < elems.size(); i += 2) {
            if (elems[i] == key) { elems[i + 1] = val; found = true; break; }
        }
        if (!found) { elems.push_back(key); elems.push_back(val); }
        std::string result;
        for (size_t i = 0; i < elems.size(); i++) {
            if (i > 0) result += ' ';
            auto &s = elems[i];
            bool needsQ = s.empty();
            for (char c : s) if (c == ' ' || c == '\t') { needsQ = true; break; }
            if (needsQ) { result += '{'; result += s; result += '}'; }
            else result += s;
        }
        Tcl_SetVar(interp, varName, result.c_str(), 0);
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "incr") == 0) {
        if (objc < 4 || objc > 5) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        const char *varName = Tcl_GetString(objv[2]);
        const char *key = Tcl_GetString(objv[3]);
        int increment = 1;
        if (objc == 5) {
            if (Tcl_GetIntFromObj(interp, objv[4], &increment) != TCL_OK) return TCL_ERROR;
        }
        const char *cur = Tcl_GetVar(interp, varName, 0);
        std::vector<std::string> elems;
        if (cur && *cur) elems = parseList(cur);
        bool found = false;
        for (size_t i = 0; i + 1 < elems.size(); i += 2) {
            if (elems[i] == key) {
                int val = atoi(elems[i + 1].c_str()) + increment;
                elems[i + 1] = std::to_string(val);
                found = true;
                break;
            }
        }
        if (!found) {
            elems.push_back(key);
            elems.push_back(std::to_string(increment));
        }
        std::string result;
        for (size_t i = 0; i < elems.size(); i++) {
            if (i > 0) result += ' ';
            result += elems[i];
        }
        Tcl_SetVar(interp, varName, result.c_str(), 0);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(found ? atoi(elems[0].c_str()) : increment));
        // Return the new value of the key
        for (size_t i = 0; i + 1 < elems.size(); i += 2) {
            if (elems[i] == key) {
                Tcl_SetObjResult(interp, Tcl_NewIntObj(atoi(elems[i + 1].c_str())));
                break;
            }
        }
        return TCL_OK;
    }

    if (strcmp(sub, "keys") == 0) {
        if (objc < 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto elems = parseList(Tcl_GetString(objv[2]));
        std::vector<std::string> keys;
        for (size_t i = 0; i + 1 < elems.size(); i += 2) keys.push_back(elems[i]);
        std::string result;
        for (size_t i = 0; i < keys.size(); i++) {
            if (i > 0) result += ' ';
            result += keys[i];
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "values") == 0) {
        if (objc < 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto elems = parseList(Tcl_GetString(objv[2]));
        std::vector<std::string> vals;
        for (size_t i = 0; i + 1 < elems.size(); i += 2) vals.push_back(elems[i + 1]);
        std::string result;
        for (size_t i = 0; i < vals.size(); i++) {
            if (i > 0) result += ' ';
            result += vals[i];
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "exists") == 0) {
        if (objc != 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto elems = parseList(Tcl_GetString(objv[2]));
        const char *key = Tcl_GetString(objv[3]);
        for (size_t i = 0; i + 1 < elems.size(); i += 2) {
            if (elems[i] == key) {
                Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
                return TCL_OK;
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }

    if (strcmp(sub, "size") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto elems = parseList(Tcl_GetString(objv[2]));
        Tcl_SetObjResult(interp, Tcl_NewIntObj(elems.size() / 2));
        return TCL_OK;
    }

    if (strcmp(sub, "for") == 0) {
        if (objc != 5) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto vars = parseList(Tcl_GetString(objv[2]));
        if (vars.size() != 2) { getImpl(interp)->result = "must have two variable names"; return TCL_ERROR; }
        auto elems = parseList(Tcl_GetString(objv[3]));
        const char *body = Tcl_GetString(objv[4]);
        for (size_t i = 0; i + 1 < elems.size(); i += 2) {
            Tcl_SetVar(interp, vars[0].c_str(), elems[i].c_str(), 0);
            Tcl_SetVar(interp, vars[1].c_str(), elems[i + 1].c_str(), 0);
            int code = Tcl_Eval(interp, body);
            if (code == TCL_BREAK) break;
            if (code == TCL_CONTINUE) continue;
            if (code != TCL_OK) return code;
        }
        Tcl_ResetResult(interp);
        return TCL_OK;
    }

    getImpl(interp)->result = std::string("unknown dict subcommand \"") + sub + "\"";
    return TCL_ERROR;
}

// ============================================================
// array command
// ============================================================

static int arrayCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc < 3) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }
    const char *sub = Tcl_GetString(objv[1]);
    const char *arrName = Tcl_GetString(objv[2]);
    auto *impl = getImpl(interp);
    std::string prefix = std::string(arrName) + "(";

    if (strcmp(sub, "exists") == 0) {
        // Check if any array element exists
        auto &vars = impl->callStack.empty() ? impl->globals
                                              : impl->callStack.back().locals;
        for (auto &[k, v] : vars) {
            if (k.find(prefix) == 0) {
                Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
                return TCL_OK;
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }

    if (strcmp(sub, "names") == 0) {
        auto &vars = impl->callStack.empty() ? impl->globals
                                              : impl->callStack.back().locals;
        std::string result;
        for (auto &[k, v] : vars) {
            if (k.find(prefix) == 0) {
                std::string key = k.substr(prefix.size());
                if (!key.empty() && key.back() == ')') key.pop_back();
                if (!result.empty()) result += ' ';
                result += key;
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "size") == 0) {
        auto &vars = impl->callStack.empty() ? impl->globals
                                              : impl->callStack.back().locals;
        int count = 0;
        for (auto &[k, v] : vars) {
            if (k.find(prefix) == 0) count++;
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
        return TCL_OK;
    }

    if (strcmp(sub, "set") == 0) {
        if (objc != 4) { impl->result = "wrong # args"; return TCL_ERROR; }
        auto elems = parseList(Tcl_GetString(objv[3]));
        for (size_t i = 0; i + 1 < elems.size(); i += 2) {
            std::string fullName = prefix + elems[i] + ")";
            Tcl_SetVar(interp, fullName.c_str(), elems[i + 1].c_str(), 0);
        }
        return TCL_OK;
    }

    if (strcmp(sub, "get") == 0) {
        auto &vars = impl->callStack.empty() ? impl->globals
                                              : impl->callStack.back().locals;
        std::string result;
        for (auto &[k, v] : vars) {
            if (k.find(prefix) == 0) {
                std::string key = k.substr(prefix.size());
                if (!key.empty() && key.back() == ')') key.pop_back();
                if (!result.empty()) result += ' ';
                result += key;
                result += ' ';
                // Quote values that contain spaces
                bool needsQ = v.empty();
                for (char c : v) if (c == ' ' || c == '\t') { needsQ = true; break; }
                if (needsQ) { result += '{'; result += v; result += '}'; }
                else result += v;
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "unset") == 0) {
        if (objc < 4) { impl->result = "wrong # args"; return TCL_ERROR; }
        for (int i = 3; i < objc; i++) {
            std::string fullName = prefix + Tcl_GetString(objv[i]) + ")";
            impl->unsetVar(fullName);
        }
        return TCL_OK;
    }

    impl->result = std::string("unknown array subcommand \"") + sub + "\"";
    return TCL_ERROR;
}

// ============================================================
// info command
// ============================================================

static int infoCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }
    auto *impl = getImpl(interp);
    const char *sub = Tcl_GetString(objv[1]);

    // "info exist" is a common abbreviation of "info exists" in real Tcl
    if (strcmp(sub, "exists") == 0 || strcmp(sub, "exist") == 0) {
        if (objc != 3) { impl->result = "wrong # args"; return TCL_ERROR; }
        const char *varName = Tcl_GetString(objv[2]);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(impl->getVar(varName) != nullptr ? 1 : 0));
        return TCL_OK;
    }

    if (strcmp(sub, "commands") == 0) {
        std::string pattern = objc >= 3 ? Tcl_GetString(objv[2]) : "*";
        std::string result;
        for (auto &[name, cmd] : impl->commands) {
            if (Tcl_StringMatch(name.c_str(), pattern.c_str())) {
                if (!result.empty()) result += ' ';
                result += name;
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "procs") == 0) {
        std::string result;
        for (auto &[name, def] : impl->procs) {
            if (!result.empty()) result += ' ';
            result += name;
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "body") == 0) {
        if (objc != 3) { impl->result = "wrong # args"; return TCL_ERROR; }
        auto it = impl->procs.find(Tcl_GetString(objv[2]));
        if (it == impl->procs.end()) {
            impl->result = std::string("\"") + Tcl_GetString(objv[2]) + "\" isn't a procedure";
            return TCL_ERROR;
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(it->second.body.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "args") == 0) {
        if (objc != 3) { impl->result = "wrong # args"; return TCL_ERROR; }
        auto it = impl->procs.find(Tcl_GetString(objv[2]));
        if (it == impl->procs.end()) {
            impl->result = std::string("\"") + Tcl_GetString(objv[2]) + "\" isn't a procedure";
            return TCL_ERROR;
        }
        std::string result;
        for (size_t i = 0; i < it->second.params.size(); i++) {
            if (i > 0) result += ' ';
            result += it->second.params[i];
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "level") == 0) {
        if (objc == 2) {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(impl->callStack.size()));
            return TCL_OK;
        }
        // info level N - not implemented
        Tcl_SetObjResult(interp, Tcl_NewStringObj("", 0));
        return TCL_OK;
    }

    if (strcmp(sub, "script") == 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("", 0));
        return TCL_OK;
    }

    if (strcmp(sub, "nameofexecutable") == 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("", 0));
        return TCL_OK;
    }

    if (strcmp(sub, "frame") == 0) {
        if (objc == 2) {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(impl->callStack.size()));
            return TCL_OK;
        }
        // Return a dict with type, line, and cmd keys
        // minitcl doesn't track source line numbers per frame,
        // so return type "eval" with line 0 to avoid false matches
        // on "source" type in sdc_file_line
        Tcl_SetObjResult(interp, Tcl_NewStringObj("type eval line 0 cmd {}", -1));
        return TCL_OK;
    }

    if (strcmp(sub, "globals") == 0 || strcmp(sub, "vars") == 0) {
        std::string pattern = objc >= 3 ? Tcl_GetString(objv[2]) : "*";
        std::string result;
        for (auto &[name, val] : impl->globals) {
            if (Tcl_StringMatch(name.c_str(), pattern.c_str())) {
                if (!result.empty()) result += ' ';
                result += name;
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "locals") == 0) {
        std::string result;
        if (!impl->callStack.empty()) {
            std::string pattern = objc >= 3 ? Tcl_GetString(objv[2]) : "*";
            for (auto &[name, val] : impl->callStack.back().locals) {
                if (Tcl_StringMatch(name.c_str(), pattern.c_str())) {
                    if (!result.empty()) result += ' ';
                    result += name;
                }
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "patchlevel") == 0 || strcmp(sub, "tclversion") == 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("9.0.0", -1));
        return TCL_OK;
    }

    if (strcmp(sub, "hostname") == 0) {
        char hostname[256] = "localhost";
        gethostname(hostname, sizeof(hostname));
        Tcl_SetObjResult(interp, Tcl_NewStringObj(hostname, -1));
        return TCL_OK;
    }

    if (strcmp(sub, "complete") == 0) {
        if (objc != 3) { impl->result = "wrong # args"; return TCL_ERROR; }
        const char *str = Tcl_GetString(objv[2]);
        // Check if the string is a complete Tcl command
        // (balanced braces, quotes, and brackets)
        int braces = 0;
        int brackets = 0;
        bool in_quote = false;
        for (const char *p = str; *p; p++) {
            if (*p == '\\' && *(p + 1)) { p++; continue; }
            if (*p == '"' && braces == 0) { in_quote = !in_quote; continue; }
            if (!in_quote) {
                if (*p == '{') braces++;
                else if (*p == '}') braces--;
                else if (braces == 0) {
                    if (*p == '[') brackets++;
                    else if (*p == ']') brackets--;
                }
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(
            braces == 0 && brackets == 0 && !in_quote ? 1 : 0));
        return TCL_OK;
    }

    impl->result = std::string("unknown info subcommand \"") + sub + "\"";
    return TCL_ERROR;
}

// ============================================================
// glob command
// ============================================================

static int globCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    bool nocomplain = false;
    std::string directory;
    int idx = 1;

    while (idx < objc) {
        const char *arg = Tcl_GetString(objv[idx]);
        if (strcmp(arg, "-nocomplain") == 0) { nocomplain = true; idx++; }
        else if (strcmp(arg, "-directory") == 0) {
            idx++;
            if (idx >= objc) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
            directory = Tcl_GetString(objv[idx]);
            idx++;
        } else if (strcmp(arg, "-types") == 0) {
            idx++;
            if (idx >= objc) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
            idx++;  // skip types value
        } else if (strcmp(arg, "--") == 0) { idx++; break; }
        else break;
    }

    if (idx >= objc) {
        if (nocomplain) { Tcl_SetObjResult(interp, Tcl_NewStringObj("", 0)); return TCL_OK; }
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }

    std::vector<std::string> results;
    for (int i = idx; i < objc; i++) {
        std::string pattern = Tcl_GetString(objv[i]);
        std::string searchDir = directory.empty() ? "." : directory;

        try {
            for (auto &entry : std::filesystem::directory_iterator(searchDir)) {
                std::string name = entry.path().filename().string();
                std::string fullPath = directory.empty()
                    ? name
                    : (std::filesystem::path(directory) / name).string();
                // Simple glob: just check with glob match
                // For now, match the filename part against the pattern
                bool match = (name == pattern);  // exact match fallback
                if (pattern.find('*') != std::string::npos || pattern.find('?') != std::string::npos) {
                    // Use glob match
                    match = false;  // TODO: implement file glob pattern
                }
                if (match) results.push_back(fullPath);
            }
        } catch (...) {
            // Directory not found
        }
    }

    if (results.empty() && !nocomplain) {
        getImpl(interp)->result = "no files matched glob pattern";
        return TCL_ERROR;
    }

    std::string result;
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) result += ' ';
        result += results[i];
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
    return TCL_OK;
}

void registerDictCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "dict", dictCmd, nullptr, nullptr);
}

void registerArrayCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "array", arrayCmd, nullptr, nullptr);
}

void registerInfoCommand(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "info", infoCmd, nullptr, nullptr);
}

void registerGlobCommand(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "glob", globCmd, nullptr, nullptr);
}

}  // namespace minitcl
