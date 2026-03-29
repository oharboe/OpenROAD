// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Control flow commands

#include "control.h"
#include "eval.h"
#include "interp.h"
#include "parser.h"

#include <cstdlib>
#include <cstring>

namespace minitcl {

// Helper: evaluate a condition string as boolean
// In Tcl, conditions are expr expressions. For now we support:
// - numeric: 0 = false, nonzero = true
// - string: "true"/"yes"/"on" = true, "false"/"no"/"off" = false
// Full expr support comes in Phase 6
// Evaluate a condition expression. In Tcl, if/while/for conditions are
// expr expressions. We first try "expr {cond}" if expr is available,
// then fall back to variable substitution + boolean interpretation.
static bool evalCondition(Tcl_Interp *interp, const char *cond, int *code) {
    *code = TCL_OK;

    // Try using expr command if it exists
    auto *impl = getImpl(interp);
    if (impl->commands.count("expr")) {
        std::string exprCmd = "expr {";
        exprCmd += cond;
        exprCmd += "}";
        *code = Tcl_Eval(interp, exprCmd.c_str());
        if (*code == TCL_OK) {
            const char *result = Tcl_GetStringResult(interp);
            char *end;
            long val = strtol(result, &end, 0);
            if (end != result && *end == '\0') return val != 0;
            double dval = strtod(result, &end);
            if (end != result && *end == '\0') return dval != 0.0;
            if (strcmp(result, "true") == 0 || strcmp(result, "yes") == 0)
                return true;
            if (strcmp(result, "false") == 0 || strcmp(result, "no") == 0)
                return false;
        }
        return false;
    }

    // No expr available: do variable substitution on the condition string,
    // then interpret the result as a boolean value.
    // First substitute variables/commands in the condition
    int subCode = TCL_OK;
    std::string substituted = substitute(interp, cond, &subCode);
    if (subCode != TCL_OK) {
        *code = subCode;
        return false;
    }
    substituted = backslashSubst(substituted);

    const char *result = substituted.c_str();

    // Try as integer
    char *end;
    long val = strtol(result, &end, 0);
    if (end != result && *end == '\0') {
        return val != 0;
    }

    // Try as boolean string
    if (strcmp(result, "true") == 0 || strcmp(result, "yes") == 0 ||
        strcmp(result, "on") == 0)
        return true;
    if (strcmp(result, "false") == 0 || strcmp(result, "no") == 0 ||
        strcmp(result, "off") == 0)
        return false;

    // Try as double
    double dval = strtod(result, &end);
    if (end != result && *end == '\0') {
        return dval != 0.0;
    }

    *code = TCL_ERROR;
    impl->result = std::string("expected boolean value but got \"") + result + "\"";
    return false;
}

static int ifCmd(ClientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const objv[]) {
    if (objc < 3) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: no expression after \"if\"";
        return TCL_ERROR;
    }

    int idx = 1;
    while (idx < objc) {
        // Skip optional "then" keyword
        const char *word = Tcl_GetString(objv[idx]);

        if (strcmp(word, "else") == 0) {
            if (idx + 1 >= objc) {
                auto *impl = getImpl(interp);
                impl->result = "wrong # args: no script following \"else\"";
                return TCL_ERROR;
            }
            return Tcl_Eval(interp, Tcl_GetString(objv[idx + 1]));
        }

        if (strcmp(word, "elseif") == 0) {
            idx++;
            if (idx >= objc) {
                auto *impl = getImpl(interp);
                impl->result = "wrong # args: no expression after \"elseif\"";
                return TCL_ERROR;
            }
            word = Tcl_GetString(objv[idx]);
        }

        // Evaluate condition
        int code;
        bool cond = evalCondition(interp, word, &code);
        if (code != TCL_OK) return code;

        idx++;

        // Skip optional "then"
        if (idx < objc && strcmp(Tcl_GetString(objv[idx]), "then") == 0) {
            idx++;
        }

        if (idx >= objc) {
            auto *impl = getImpl(interp);
            impl->result = "wrong # args: no script following expression";
            return TCL_ERROR;
        }

        if (cond) {
            return Tcl_Eval(interp, Tcl_GetString(objv[idx]));
        }

        idx++;  // skip the body, move to next elseif/else
    }

    // No condition matched, no else
    Tcl_ResetResult(interp);
    return TCL_OK;
}

static int whileCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc != 3) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"while test command\"";
        return TCL_ERROR;
    }

    const char *test = Tcl_GetString(objv[1]);
    const char *body = Tcl_GetString(objv[2]);

    while (true) {
        int code;
        bool cond = evalCondition(interp, test, &code);
        if (code != TCL_OK) return code;
        if (!cond) break;

        code = Tcl_Eval(interp, body);
        if (code == TCL_BREAK) break;
        if (code == TCL_CONTINUE) continue;
        if (code != TCL_OK) return code;
    }

    Tcl_ResetResult(interp);
    return TCL_OK;
}

static int forCmd(ClientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const objv[]) {
    if (objc != 5) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"for start test next command\"";
        return TCL_ERROR;
    }

    const char *start = Tcl_GetString(objv[1]);
    const char *test = Tcl_GetString(objv[2]);
    const char *next = Tcl_GetString(objv[3]);
    const char *body = Tcl_GetString(objv[4]);

    int code = Tcl_Eval(interp, start);
    if (code != TCL_OK) return code;

    while (true) {
        bool cond = evalCondition(interp, test, &code);
        if (code != TCL_OK) return code;
        if (!cond) break;

        code = Tcl_Eval(interp, body);
        if (code == TCL_BREAK) break;
        if (code == TCL_CONTINUE) { /* fall through to next */ }
        else if (code != TCL_OK) return code;

        code = Tcl_Eval(interp, next);
        if (code != TCL_OK) return code;
    }

    Tcl_ResetResult(interp);
    return TCL_OK;
}

static int foreachCmd(ClientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const objv[]) {
    if (objc != 4) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"foreach varName list body\"";
        return TCL_ERROR;
    }

    const char *varName = Tcl_GetString(objv[1]);
    const char *listStr = Tcl_GetString(objv[2]);
    const char *body = Tcl_GetString(objv[3]);

    // Parse list into elements
    auto listCmds = parseScript(listStr);
    std::vector<std::string> elements;
    for (const auto &cmd : listCmds) {
        for (const auto &word : cmd.words) {
            elements.push_back(word.text);
        }
    }

    for (const auto &elem : elements) {
        Tcl_SetVar(interp, varName, elem.c_str(), 0);
        int code = Tcl_Eval(interp, body);
        if (code == TCL_BREAK) break;
        if (code == TCL_CONTINUE) continue;
        if (code != TCL_OK) return code;
    }

    Tcl_ResetResult(interp);
    return TCL_OK;
}

static int incrCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2 || objc > 3) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"incr varName ?increment?\"";
        return TCL_ERROR;
    }

    const char *varName = Tcl_GetString(objv[1]);
    int increment = 1;
    if (objc == 3) {
        if (Tcl_GetInt(interp, Tcl_GetString(objv[2]), &increment) != TCL_OK)
            return TCL_ERROR;
    }

    const char *curVal = Tcl_GetVar(interp, varName, 0);
    int current = 0;
    if (curVal) {
        if (Tcl_GetInt(interp, curVal, &current) != TCL_OK) return TCL_ERROR;
    }

    current += increment;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", current);
    Tcl_SetVar(interp, varName, buf, 0);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(current));
    return TCL_OK;
}

static int switchCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc < 3) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"switch ?options? string {pattern body ...}\"";
        return TCL_ERROR;
    }

    int strIdx = 1;
    bool exact = true;
    (void)exact;

    // Skip options
    while (strIdx < objc - 1) {
        const char *opt = Tcl_GetString(objv[strIdx]);
        if (opt[0] != '-') break;
        if (strcmp(opt, "-exact") == 0) { exact = true; strIdx++; }
        else if (strcmp(opt, "-glob") == 0) { exact = false; strIdx++; }
        else if (strcmp(opt, "--") == 0) { strIdx++; break; }
        else break;
    }

    if (strIdx >= objc) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args";
        return TCL_ERROR;
    }

    const char *str = Tcl_GetString(objv[strIdx]);
    strIdx++;

    // Two forms: switch string {pat1 body1 pat2 body2 ...}
    // or: switch string pat1 body1 pat2 body2 ...
    if (strIdx == objc - 1) {
        // Single arg: parse as list of pattern/body pairs
        auto pairs = parseScript(Tcl_GetString(objv[strIdx]));
        std::vector<std::string> words;
        for (const auto &cmd : pairs) {
            for (const auto &w : cmd.words) {
                words.push_back(w.text);
            }
        }

        for (size_t i = 0; i + 1 < words.size(); i += 2) {
            if (words[i] == "default" || words[i] == str) {
                return Tcl_Eval(interp, words[i + 1].c_str());
            }
        }
    } else {
        // Multiple args
        for (int i = strIdx; i + 1 < objc; i += 2) {
            const char *pat = Tcl_GetString(objv[i]);
            if (strcmp(pat, "default") == 0 || strcmp(pat, str) == 0) {
                return Tcl_Eval(interp, Tcl_GetString(objv[i + 1]));
            }
        }
    }

    Tcl_ResetResult(interp);
    return TCL_OK;
}

static int catchCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc < 2 || objc > 3) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"catch script ?resultVarName?\"";
        return TCL_ERROR;
    }

    int code = Tcl_Eval(interp, Tcl_GetString(objv[1]));

    if (objc == 3) {
        const char *resultVarName = Tcl_GetString(objv[2]);
        const char *res = Tcl_GetStringResult(interp);
        Tcl_SetVar(interp, resultVarName, res, 0);
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(code));
    return TCL_OK;
}

static int breakCmd(ClientData, Tcl_Interp *, int, Tcl_Obj *const[]) {
    return TCL_BREAK;
}

static int continueCmd(ClientData, Tcl_Interp *, int, Tcl_Obj *const[]) {
    return TCL_CONTINUE;
}

void registerControlCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "if", ifCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "while", whileCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "for", forCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "foreach", foreachCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "incr", incrCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "switch", switchCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "catch", catchCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "break", breakCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "continue", continueCmd, nullptr, nullptr);
}

}  // namespace minitcl
