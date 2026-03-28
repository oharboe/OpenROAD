// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Regexp support backed by std::regex

#include "regexp.h"
#include "interp.h"
#include "string_cmds.h"

#include <cstring>
#include <regex>
#include <string>
#include <vector>

namespace minitcl {

// Internal compiled regexp storage
struct CompiledRegexp {
    std::regex re;
    std::string pattern;
    int flags;
};

// Global storage for compiled regexps (simple approach)
static std::vector<CompiledRegexp *> compiledRegexps;

static int regexpCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc < 3) {
        getImpl(interp)->result = "wrong # args: should be \"regexp ?switches? exp string ?matchVar? ?subMatchVar ...?\"";
        return TCL_ERROR;
    }

    bool nocase = false;
    int idx = 1;

    // Parse switches
    while (idx < objc) {
        const char *arg = Tcl_GetString(objv[idx]);
        if (arg[0] != '-') break;
        if (strcmp(arg, "-nocase") == 0) { nocase = true; idx++; }
        else if (strcmp(arg, "--") == 0) { idx++; break; }
        else { idx++; }  // skip unknown switches
    }

    if (idx + 1 >= objc) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }

    const char *pattern = Tcl_GetString(objv[idx++]);
    const char *str = Tcl_GetString(objv[idx++]);

    try {
        auto flags = std::regex::extended;
        if (nocase) flags |= std::regex::icase;
        std::regex re(pattern, flags);
        std::cmatch match;
        bool found = std::regex_search(str, match, re);

        // Store match variables if requested
        if (found && idx < objc) {
            // First match var gets the whole match
            Tcl_SetVar(interp, Tcl_GetString(objv[idx]), match[0].str().c_str(), 0);
            idx++;
            // Subsequent vars get submatches
            for (size_t i = 1; i < match.size() && idx < objc; i++, idx++) {
                Tcl_SetVar(interp, Tcl_GetString(objv[idx]), match[i].str().c_str(), 0);
            }
        }

        Tcl_SetObjResult(interp, Tcl_NewIntObj(found ? 1 : 0));
        return TCL_OK;
    } catch (const std::regex_error &e) {
        getImpl(interp)->result = std::string("couldn't compile regular expression: ") + e.what();
        return TCL_ERROR;
    }
}

static int regsubCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc < 4) {
        getImpl(interp)->result = "wrong # args: should be \"regsub ?switches? exp string subSpec ?varName?\"";
        return TCL_ERROR;
    }

    bool all = false;
    bool nocase = false;
    int idx = 1;

    while (idx < objc) {
        const char *arg = Tcl_GetString(objv[idx]);
        if (arg[0] != '-') break;
        if (strcmp(arg, "-all") == 0) { all = true; idx++; }
        else if (strcmp(arg, "-nocase") == 0) { nocase = true; idx++; }
        else if (strcmp(arg, "--") == 0) { idx++; break; }
        else { idx++; }
    }

    if (idx + 2 >= objc) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }

    const char *pattern = Tcl_GetString(objv[idx++]);
    const char *str = Tcl_GetString(objv[idx++]);
    const char *subSpec = Tcl_GetString(objv[idx++]);

    try {
        auto flags = std::regex::extended;
        if (nocase) flags |= std::regex::icase;
        std::regex re(pattern, flags);

        std::string result;
        if (all) {
            result = std::regex_replace(str, re, subSpec);
        } else {
            result = std::regex_replace(str, re, subSpec,
                                         std::regex_constants::format_first_only);
        }

        if (idx < objc) {
            // Store result in variable, return count
            Tcl_SetVar(interp, Tcl_GetString(objv[idx]), result.c_str(), 0);
            // Count matches
            auto begin = std::cregex_iterator(str, str + strlen(str), re);
            auto end = std::cregex_iterator();
            int count = std::distance(begin, end);
            if (!all && count > 0) count = 1;
            Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
        } else {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
        }
        return TCL_OK;
    } catch (const std::regex_error &e) {
        getImpl(interp)->result = std::string("couldn't compile regular expression: ") + e.what();
        return TCL_ERROR;
    }
}

void registerRegexpCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "regexp", regexpCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "regsub", regsubCmd, nullptr, nullptr);
}

}  // namespace minitcl

// ============================================================
// C API: Tcl_GetRegExpFromObj / Tcl_RegExpExec
// ============================================================

Tcl_RegExp Tcl_GetRegExpFromObj(Tcl_Interp *interp, Tcl_Obj *patObj,
                                 int flags) {
    const char *pattern = Tcl_GetString(patObj);
    try {
        auto reFlags = std::regex::extended;
        if (flags & TCL_REG_NOCASE) reFlags |= std::regex::icase;
        auto *compiled = new minitcl::CompiledRegexp();
        compiled->re = std::regex(pattern, reFlags);
        compiled->pattern = pattern;
        compiled->flags = flags;
        minitcl::compiledRegexps.push_back(compiled);
        return reinterpret_cast<Tcl_RegExp>(compiled);
    } catch (const std::regex_error &e) {
        if (interp) {
            auto *impl = minitcl::getImpl(interp);
            impl->result = std::string("couldn't compile regular expression: ") + e.what();
        }
        return nullptr;
    }
}

int Tcl_RegExpExec(Tcl_Interp *, Tcl_RegExp regexp,
                    const char *text, const char *) {
    if (!regexp || !text) return 0;
    auto *compiled = reinterpret_cast<minitcl::CompiledRegexp *>(regexp);
    try {
        return std::regex_search(text, compiled->re) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}
