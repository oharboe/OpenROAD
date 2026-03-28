// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - String and list commands

#include "string_cmds.h"
#include "interp.h"
#include "parser.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace minitcl {

// ============================================================
// List parsing utility
// ============================================================

std::vector<std::string> parseList(const char *listStr) {
    std::vector<std::string> elements;
    auto cmds = parseScript(listStr);
    for (const auto &cmd : cmds) {
        for (const auto &w : cmd.words) {
            elements.push_back(w.text);
        }
    }
    return elements;
}

// Build Tcl list string from elements
static std::string buildList(const std::vector<std::string> &elems) {
    std::string result;
    for (size_t i = 0; i < elems.size(); i++) {
        if (i > 0) result += ' ';
        const auto &s = elems[i];
        bool needsQuoting = s.empty();
        if (!needsQuoting) {
            for (char c : s) {
                if (c == ' ' || c == '\t' || c == '\n' || c == '{' ||
                    c == '}' || c == '"' || c == '\\') {
                    needsQuoting = true;
                    break;
                }
            }
        }
        if (needsQuoting) {
            result += '{';
            result += s;
            result += '}';
        } else {
            result += s;
        }
    }
    return result;
}

// Simple glob pattern matching (*, ?)
static bool globMatch(const char *p, const char *s) {
    while (*p && *s) {
        if (*p == '*') {
            if (*(p + 1) == '\0') return true;
            while (*s) {
                if (globMatch(p + 1, s)) return true;
                s++;
            }
            return globMatch(p + 1, s);
        } else if (*p == '?' || *p == *s) {
            p++;
            s++;
        } else {
            return false;
        }
    }
    while (*p == '*') p++;
    return *p == '\0' && *s == '\0';
}

// ============================================================
// string command
// ============================================================

static int stringCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc < 2) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"string subcommand ?arg ...?\"";
        return TCL_ERROR;
    }

    const char *sub = Tcl_GetString(objv[1]);

    if (strcmp(sub, "length") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(strlen(Tcl_GetString(objv[2]))));
        return TCL_OK;
    }

    if (strcmp(sub, "index") == 0) {
        if (objc != 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        const char *s = Tcl_GetString(objv[2]);
        int idx = 0;
        const char *idxStr = Tcl_GetString(objv[3]);
        if (strcmp(idxStr, "end") == 0) idx = strlen(s) - 1;
        else if (strncmp(idxStr, "end-", 4) == 0) idx = strlen(s) - 1 - atoi(idxStr + 4);
        else idx = atoi(idxStr);
        if (idx < 0 || idx >= (int)strlen(s)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("", 0));
        } else {
            char buf[2] = {s[idx], '\0'};
            Tcl_SetObjResult(interp, Tcl_NewStringObj(buf, 1));
        }
        return TCL_OK;
    }

    if (strcmp(sub, "range") == 0) {
        if (objc != 5) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        const char *s = Tcl_GetString(objv[2]);
        int len = strlen(s);
        int first = 0, last = 0;
        const char *f = Tcl_GetString(objv[3]);
        const char *l = Tcl_GetString(objv[4]);
        if (strcmp(f, "end") == 0) first = len - 1;
        else if (strncmp(f, "end-", 4) == 0) first = len - 1 - atoi(f + 4);
        else first = atoi(f);
        if (strcmp(l, "end") == 0) last = len - 1;
        else if (strncmp(l, "end-", 4) == 0) last = len - 1 - atoi(l + 4);
        else last = atoi(l);
        if (first < 0) first = 0;
        if (last >= len) last = len - 1;
        if (first > last) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("", 0));
        } else {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(s + first, last - first + 1));
        }
        return TCL_OK;
    }

    if (strcmp(sub, "equal") == 0 || strcmp(sub, "compare") == 0) {
        if (objc != 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        int cmp = strcmp(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]));
        if (strcmp(sub, "equal") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj(cmp == 0 ? 1 : 0));
        else
            Tcl_SetObjResult(interp, Tcl_NewIntObj(cmp < 0 ? -1 : (cmp > 0 ? 1 : 0)));
        return TCL_OK;
    }

    if (strcmp(sub, "match") == 0) {
        if (objc < 4 || objc > 5) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        bool nocase = false;
        int patIdx = 2, strIdx = 3;
        if (objc == 5 && strcmp(Tcl_GetString(objv[2]), "-nocase") == 0) {
            nocase = true;
            patIdx = 3;
            strIdx = 4;
        }
        std::string pattern = Tcl_GetString(objv[patIdx]);
        std::string str = Tcl_GetString(objv[strIdx]);
        if (nocase) {
            std::transform(pattern.begin(), pattern.end(), pattern.begin(), ::tolower);
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(globMatch(pattern.c_str(), str.c_str()) ? 1 : 0));
        return TCL_OK;
    }

    if (strcmp(sub, "map") == 0) {
        if (objc != 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto mapping = parseList(Tcl_GetString(objv[2]));
        std::string str = Tcl_GetString(objv[3]);
        for (size_t i = 0; i + 1 < mapping.size(); i += 2) {
            const auto &from = mapping[i];
            const auto &to = mapping[i + 1];
            size_t pos = 0;
            while ((pos = str.find(from, pos)) != std::string::npos) {
                str.replace(pos, from.size(), to);
                pos += to.size();
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(str.c_str(), str.size()));
        return TCL_OK;
    }

    if (strcmp(sub, "trim") == 0 || strcmp(sub, "trimleft") == 0 ||
        strcmp(sub, "trimright") == 0) {
        if (objc < 3 || objc > 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        std::string s = Tcl_GetString(objv[2]);
        std::string chars = " \t\n\r";
        if (objc == 4) chars = Tcl_GetString(objv[3]);
        if (strcmp(sub, "trim") == 0 || strcmp(sub, "trimleft") == 0) {
            size_t start = s.find_first_not_of(chars);
            if (start == std::string::npos) s = "";
            else s = s.substr(start);
        }
        if (strcmp(sub, "trim") == 0 || strcmp(sub, "trimright") == 0) {
            size_t end = s.find_last_not_of(chars);
            if (end == std::string::npos) s = "";
            else s = s.substr(0, end + 1);
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(s.c_str(), s.size()));
        return TCL_OK;
    }

    if (strcmp(sub, "tolower") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        std::string s = Tcl_GetString(objv[2]);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        Tcl_SetObjResult(interp, Tcl_NewStringObj(s.c_str(), s.size()));
        return TCL_OK;
    }

    if (strcmp(sub, "toupper") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        std::string s = Tcl_GetString(objv[2]);
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        Tcl_SetObjResult(interp, Tcl_NewStringObj(s.c_str(), s.size()));
        return TCL_OK;
    }

    if (strcmp(sub, "first") == 0) {
        if (objc < 4 || objc > 5) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        const char *needle = Tcl_GetString(objv[2]);
        const char *haystack = Tcl_GetString(objv[3]);
        int startIdx = 0;
        if (objc == 5) startIdx = atoi(Tcl_GetString(objv[4]));
        const char *found = strstr(haystack + startIdx, needle);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(found ? (int)(found - haystack) : -1));
        return TCL_OK;
    }

    if (strcmp(sub, "last") == 0) {
        if (objc < 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        std::string needle = Tcl_GetString(objv[2]);
        std::string haystack = Tcl_GetString(objv[3]);
        size_t pos = haystack.rfind(needle);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(pos != std::string::npos ? (int)pos : -1));
        return TCL_OK;
    }

    if (strcmp(sub, "repeat") == 0) {
        if (objc != 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        const char *s = Tcl_GetString(objv[2]);
        int count = atoi(Tcl_GetString(objv[3]));
        std::string result;
        for (int i = 0; i < count; i++) result += s;
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
        return TCL_OK;
    }

    if (strcmp(sub, "replace") == 0) {
        if (objc < 5 || objc > 6) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        std::string s = Tcl_GetString(objv[2]);
        int len = s.size();
        int first = atoi(Tcl_GetString(objv[3]));
        const char *lastStr = Tcl_GetString(objv[4]);
        int last = strcmp(lastStr, "end") == 0 ? len - 1 : atoi(lastStr);
        std::string replacement = objc == 6 ? Tcl_GetString(objv[5]) : "";
        if (first < 0) first = 0;
        if (last >= len) last = len - 1;
        if (first <= last && first < len) {
            s = s.substr(0, first) + replacement + s.substr(last + 1);
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(s.c_str(), s.size()));
        return TCL_OK;
    }

    if (strcmp(sub, "is") == 0) {
        if (objc < 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        const char *type = Tcl_GetString(objv[2]);
        // Skip -strict flag if present
        int valIdx = 3;
        if (objc > 4 && strcmp(Tcl_GetString(objv[3]), "-strict") == 0) valIdx = 4;
        if (valIdx >= objc) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        const char *val = Tcl_GetString(objv[valIdx]);
        int result = 0;

        if (strcmp(type, "integer") == 0) {
            char *end;
            strtol(val, &end, 0);
            result = (end != val && *end == '\0') ? 1 : 0;
        } else if (strcmp(type, "double") == 0) {
            char *end;
            strtod(val, &end);
            result = (end != val && *end == '\0') ? 1 : 0;
        } else if (strcmp(type, "boolean") == 0) {
            result = (strcmp(val, "0") == 0 || strcmp(val, "1") == 0 ||
                      strcmp(val, "true") == 0 || strcmp(val, "false") == 0 ||
                      strcmp(val, "yes") == 0 || strcmp(val, "no") == 0 ||
                      strcmp(val, "on") == 0 || strcmp(val, "off") == 0) ? 1 : 0;
        } else if (strcmp(type, "alpha") == 0) {
            result = (*val != '\0') ? 1 : 0;
            for (const char *p = val; *p; p++) { if (!isalpha(*p)) { result = 0; break; } }
        } else if (strcmp(type, "digit") == 0) {
            result = (*val != '\0') ? 1 : 0;
            for (const char *p = val; *p; p++) { if (!isdigit(*p)) { result = 0; break; } }
        } else if (strcmp(type, "space") == 0) {
            result = (*val != '\0') ? 1 : 0;
            for (const char *p = val; *p; p++) { if (!isspace(*p)) { result = 0; break; } }
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
        return TCL_OK;
    }

    if (strcmp(sub, "cat") == 0) {
        std::string result;
        for (int i = 2; i < objc; i++) result += Tcl_GetString(objv[i]);
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
        return TCL_OK;
    }

    getImpl(interp)->result = std::string("unknown or ambiguous subcommand \"") + sub + "\"";
    return TCL_ERROR;
}

// ============================================================
// List commands
// ============================================================

static int listCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    std::vector<std::string> elems;
    for (int i = 1; i < objc; i++) elems.push_back(Tcl_GetString(objv[i]));
    std::string result = buildList(elems);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
    return TCL_OK;
}

static int llengthCmd(ClientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const objv[]) {
    if (objc != 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    auto elems = parseList(Tcl_GetString(objv[1]));
    Tcl_SetObjResult(interp, Tcl_NewIntObj(elems.size()));
    return TCL_OK;
}

static int lindexCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    auto elems = parseList(Tcl_GetString(objv[1]));
    const char *idxStr = Tcl_GetString(objv[2]);
    int idx;
    if (strcmp(idxStr, "end") == 0) idx = elems.size() - 1;
    else if (strncmp(idxStr, "end-", 4) == 0) idx = elems.size() - 1 - atoi(idxStr + 4);
    else idx = atoi(idxStr);
    if (idx < 0 || idx >= (int)elems.size()) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("", 0));
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(elems[idx].c_str(), -1));
    }
    return TCL_OK;
}

static int lrangeCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc != 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    auto elems = parseList(Tcl_GetString(objv[1]));
    int len = elems.size();
    const char *f = Tcl_GetString(objv[2]);
    const char *l = Tcl_GetString(objv[3]);
    int first = strcmp(f, "end") == 0 ? len - 1 : (strncmp(f, "end-", 4) == 0 ? len - 1 - atoi(f + 4) : atoi(f));
    int last = strcmp(l, "end") == 0 ? len - 1 : (strncmp(l, "end-", 4) == 0 ? len - 1 - atoi(l + 4) : atoi(l));
    if (first < 0) first = 0;
    if (last >= len) last = len - 1;
    std::vector<std::string> sub;
    for (int i = first; i <= last; i++) sub.push_back(elems[i]);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buildList(sub).c_str(), -1));
    return TCL_OK;
}

static int lappendCmd(ClientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const objv[]) {
    if (objc < 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    const char *varName = Tcl_GetString(objv[1]);
    const char *cur = Tcl_GetVar(interp, varName, 0);
    std::vector<std::string> elems;
    if (cur && *cur) elems = parseList(cur);
    for (int i = 2; i < objc; i++) elems.push_back(Tcl_GetString(objv[i]));
    std::string result = buildList(elems);
    Tcl_SetVar(interp, varName, result.c_str(), 0);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
    return TCL_OK;
}

static int lsortCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc < 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    bool decreasing = false;
    bool dictionary = false;
    bool integer = false;
    bool real = false;
    int listIdx = objc - 1;
    for (int i = 1; i < objc - 1; i++) {
        const char *opt = Tcl_GetString(objv[i]);
        if (strcmp(opt, "-decreasing") == 0) decreasing = true;
        else if (strcmp(opt, "-increasing") == 0) decreasing = false;
        else if (strcmp(opt, "-dictionary") == 0) dictionary = true;
        else if (strcmp(opt, "-integer") == 0) integer = true;
        else if (strcmp(opt, "-real") == 0) real = true;
    }
    auto elems = parseList(Tcl_GetString(objv[listIdx]));
    if (integer) {
        std::sort(elems.begin(), elems.end(), [](const std::string &a, const std::string &b) {
            return atoi(a.c_str()) < atoi(b.c_str());
        });
    } else if (real) {
        std::sort(elems.begin(), elems.end(), [](const std::string &a, const std::string &b) {
            return atof(a.c_str()) < atof(b.c_str());
        });
    } else if (dictionary) {
        std::sort(elems.begin(), elems.end(), [](const std::string &a, const std::string &b) {
            return strcasecmp(a.c_str(), b.c_str()) < 0;
        });
    } else {
        std::sort(elems.begin(), elems.end());
    }
    if (decreasing) std::reverse(elems.begin(), elems.end());
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buildList(elems).c_str(), -1));
    return TCL_OK;
}

static int lsearchCmd(ClientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const objv[]) {
    if (objc < 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    auto elems = parseList(Tcl_GetString(objv[objc - 2]));
    const char *pattern = Tcl_GetString(objv[objc - 1]);
    bool exact = false;
    for (int i = 1; i < objc - 2; i++) {
        if (strcmp(Tcl_GetString(objv[i]), "-exact") == 0) exact = true;
    }
    for (size_t i = 0; i < elems.size(); i++) {
        if (exact ? elems[i] == pattern : elems[i] == pattern) {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(i));
            return TCL_OK;
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(-1));
    return TCL_OK;
}

static int concatCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    std::string result;
    for (int i = 1; i < objc; i++) {
        if (i > 1) result += ' ';
        // Trim leading/trailing whitespace from each arg
        std::string s = Tcl_GetString(objv[i]);
        size_t start = s.find_first_not_of(" \t\n");
        size_t end = s.find_last_not_of(" \t\n");
        if (start != std::string::npos) result += s.substr(start, end - start + 1);
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
    return TCL_OK;
}

static int lassignCmd(ClientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const objv[]) {
    if (objc < 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    auto elems = parseList(Tcl_GetString(objv[1]));
    size_t idx = 0;
    for (int i = 2; i < objc; i++, idx++) {
        const char *varName = Tcl_GetString(objv[i]);
        if (idx < elems.size())
            Tcl_SetVar(interp, varName, elems[idx].c_str(), 0);
        else
            Tcl_SetVar(interp, varName, "", 0);
    }
    // Return remaining elements
    std::vector<std::string> remaining(elems.begin() + std::min(idx, elems.size()), elems.end());
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buildList(remaining).c_str(), -1));
    return TCL_OK;
}

static int splitCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc < 2 || objc > 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    const char *str = Tcl_GetString(objv[1]);
    std::string splitChars = " \t\n";
    if (objc == 3) splitChars = Tcl_GetString(objv[2]);

    std::vector<std::string> elems;
    if (splitChars.empty()) {
        // Split into individual characters
        for (const char *p = str; *p; p++) {
            elems.push_back(std::string(1, *p));
        }
    } else {
        std::string current;
        for (const char *p = str; *p; p++) {
            if (splitChars.find(*p) != std::string::npos) {
                elems.push_back(current);
                current.clear();
            } else {
                current += *p;
            }
        }
        elems.push_back(current);
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buildList(elems).c_str(), -1));
    return TCL_OK;
}

static int joinCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2 || objc > 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    auto elems = parseList(Tcl_GetString(objv[1]));
    std::string sep = " ";
    if (objc == 3) sep = Tcl_GetString(objv[2]);
    std::string result;
    for (size_t i = 0; i < elems.size(); i++) {
        if (i > 0) result += sep;
        result += elems[i];
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
    return TCL_OK;
}

static int appendCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc < 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    const char *varName = Tcl_GetString(objv[1]);
    const char *cur = Tcl_GetVar(interp, varName, 0);
    std::string result = cur ? cur : "";
    for (int i = 2; i < objc; i++) result += Tcl_GetString(objv[i]);
    Tcl_SetVar(interp, varName, result.c_str(), 0);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
    return TCL_OK;
}

static int formatCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc < 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    const char *fmt = Tcl_GetString(objv[1]);
    std::string result;
    int argIdx = 2;
    for (const char *p = fmt; *p; p++) {
        if (*p == '%' && *(p + 1)) {
            p++;
            if (*p == '%') { result += '%'; continue; }
            // Parse format spec
            std::string spec = "%";
            while (*p && (*p == '-' || *p == '+' || *p == ' ' || *p == '0' || *p == '#'))
                spec += *p++;
            while (*p && isdigit(*p)) spec += *p++;
            if (*p == '.') { spec += *p++; while (*p && isdigit(*p)) spec += *p++; }
            char conv = *p;
            spec += conv;
            if (argIdx >= objc) break;
            char buf[256];
            switch (conv) {
                case 'd': case 'i': snprintf(buf, sizeof(buf), spec.c_str(), atoi(Tcl_GetString(objv[argIdx++]))); break;
                case 'f': case 'e': case 'g': case 'E': case 'G':
                    snprintf(buf, sizeof(buf), spec.c_str(), atof(Tcl_GetString(objv[argIdx++]))); break;
                case 's': snprintf(buf, sizeof(buf), spec.c_str(), Tcl_GetString(objv[argIdx++])); break;
                case 'x': case 'X': case 'o': snprintf(buf, sizeof(buf), spec.c_str(), atoi(Tcl_GetString(objv[argIdx++]))); break;
                case 'c': buf[0] = atoi(Tcl_GetString(objv[argIdx++])); buf[1] = '\0'; break;
                default: buf[0] = '\0'; break;
            }
            result += buf;
        } else {
            result += *p;
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
    return TCL_OK;
}

void registerStringCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "string", stringCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "append", appendCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "format", formatCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "split", splitCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "join", joinCmd, nullptr, nullptr);
}

void registerListCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "list", listCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "llength", llengthCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "lindex", lindexCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "lrange", lrangeCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "lappend", lappendCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "lsort", lsortCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "lsearch", lsearchCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "concat", concatCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "lassign", lassignCmd, nullptr, nullptr);
}

}  // namespace minitcl
