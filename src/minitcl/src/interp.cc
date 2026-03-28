// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Interpreter lifecycle and stubs

#include "interp.h"
#include "channel.h"
#include "eval.h"
#include "parser.h"
#include "vfs.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace minitcl {

InterpImpl *getImpl(Tcl_Interp *interp) {
    return reinterpret_cast<InterpImpl *>(interp);
}

static Tcl_Interp *toInterp(InterpImpl *impl) {
    return reinterpret_cast<Tcl_Interp *>(impl);
}

}  // namespace minitcl

// ============================================================
// Interpreter lifecycle
// ============================================================

Tcl_Interp *Tcl_CreateInterp(void) {
    auto *impl = new minitcl::InterpImpl();
    auto *interp = reinterpret_cast<Tcl_Interp *>(impl);
    minitcl::initChannels();
    minitcl::registerBuiltins(interp);

    // Populate ::env array from process environment
    extern char **environ;
    for (char **ep = environ; ep && *ep; ep++) {
        std::string entry = *ep;
        auto eq = entry.find('=');
        if (eq != std::string::npos) {
            std::string key = "env(" + entry.substr(0, eq) + ")";
            impl->globals[key] = entry.substr(eq + 1);
        }
    }

    return interp;
}

void Tcl_DeleteInterp(Tcl_Interp *interp) {
    auto *impl = minitcl::getImpl(interp);
    impl->deleted = true;

    // Call delete procs for commands
    for (auto &[name, cmd] : impl->commands) {
        if (cmd.deleteProc) {
            cmd.deleteProc(cmd.clientData);
        }
    }

    // Call assoc data delete procs
    for (auto &[name, pair] : impl->assocData) {
        if (pair.first) {
            pair.first(pair.second, interp);
        }
    }

    // Release result obj
    if (impl->resultObj) {
        Tcl_DecrRefCount(impl->resultObj);
    }

    delete impl;
}

int Tcl_Init(Tcl_Interp *) {
    // No-op: no init.tcl to load
    return TCL_OK;
}

void Tcl_Main(int argc, char **argv, Tcl_AppInitProc *appInitProc) {
    Tcl_Interp *interp = Tcl_CreateInterp();
    if (appInitProc) {
        if (appInitProc(interp) != TCL_OK) {
            fprintf(stderr, "application-specific initialization failed: %s\n",
                    Tcl_GetStringResult(interp));
            Tcl_DeleteInterp(interp);
            exit(1);
        }
    }

    // Import sta namespace commands to global scope (like real Tcl's init_sta_cmds)
    Tcl_Eval(interp, "namespace import sta::*");

    // If a script file was provided as argv[1], source it
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        int code = Tcl_EvalFile(interp, argv[1]);
        if (code != TCL_OK) {
            fprintf(stderr, "%s\n", Tcl_GetStringResult(interp));
            Tcl_DeleteInterp(interp);
            exit(1);
        }
    } else {
        // Interactive mode: simple line-by-line eval
        char line[4096];
        fprintf(stdout, "%% ");
        fflush(stdout);
        while (fgets(line, sizeof(line), stdin)) {
            // Strip trailing newline
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

            if (line[0] == '\0') {
                fprintf(stdout, "%% ");
                fflush(stdout);
                continue;
            }

            int code = Tcl_Eval(interp, line);
            const char *result = Tcl_GetStringResult(interp);
            if (result[0] != '\0') {
                fprintf(stdout, "%s\n", result);
            }
            if (code == TCL_ERROR) {
                fprintf(stderr, "Error: %s\n", result);
            }
            fprintf(stdout, "%% ");
            fflush(stdout);
        }
    }

    Tcl_DeleteInterp(interp);
}

// ============================================================
// Evaluation (stubs - implemented in Phase 3)
// ============================================================

static thread_local int evalDepth = 0;

int Tcl_Eval(Tcl_Interp *interp, const char *script) {
    if (!script || !*script) return TCL_OK;

    if (++evalDepth > 500) {
        --evalDepth;
        auto *impl = minitcl::getImpl(interp);
        impl->result = "too many nested evaluations (infinite loop?)";
        return TCL_ERROR;
    }

    auto commands = minitcl::parseScript(script);
    int code = TCL_OK;

    for (const auto &cmd : commands) {
        if (cmd.words.empty()) continue;

        // Perform substitution on each word
        std::vector<std::string> words;
        words.reserve(cmd.words.size());

        for (const auto &word : cmd.words) {
            std::string value;
            if (word.braced) {
                value = word.text;
            } else {
                int subCode = TCL_OK;
                std::string substituted =
                    minitcl::substitute(interp, word.text, &subCode);
                if (subCode != TCL_OK) { --evalDepth; return subCode; }
                value = minitcl::backslashSubst(substituted);
            }

            if (word.expand) {
                // {*} expansion: split value as a Tcl list into multiple args
                auto listCmds = minitcl::parseScript(value.c_str());
                for (const auto &lc : listCmds) {
                    for (const auto &lw : lc.words) {
                        words.push_back(lw.text);
                    }
                }
            } else {
                words.push_back(std::move(value));
            }
        }

        code = minitcl::evalCommand(interp, words);
        if (code != TCL_OK) { --evalDepth; return code; }
    }

    --evalDepth;
    return code;
}

int Tcl_EvalFile(Tcl_Interp *interp, const char *fileName) {
    // Check VFS first
    size_t vfsLen = 0;
    const char *vfsContent = minitcl_vfs_get(fileName, &vfsLen);
    if (vfsContent) {
        std::string script(vfsContent, vfsLen);
        return Tcl_Eval(interp, script.c_str());
    }

    // Fall back to real filesystem
    FILE *fp = fopen(fileName, "r");
    if (!fp) {
        auto *impl = minitcl::getImpl(interp);
        impl->result = std::string("couldn't read file \"") + fileName +
                        "\": no such file or directory";
        return TCL_ERROR;
    }
    std::string content;
    int ch;
    while ((ch = fgetc(fp)) != EOF) content += static_cast<char>(ch);
    fclose(fp);
    return Tcl_Eval(interp, content.c_str());
}

int Tcl_EvalObjEx(Tcl_Interp *interp, Tcl_Obj *objPtr, int flags) {
    (void)flags;
    if (!objPtr || !objPtr->bytes) return TCL_ERROR;
    return Tcl_Eval(interp, objPtr->bytes);
}

// ============================================================
// Result handling
// ============================================================

void Tcl_SetResult(Tcl_Interp *interp, const char *result,
                    Tcl_FreeProc *freeProc) {
    auto *impl = minitcl::getImpl(interp);
    if (result) {
        impl->result = result;
    } else {
        impl->result.clear();
    }
    // If freeProc is TCL_DYNAMIC, we took ownership of the string
    if (freeProc == TCL_DYNAMIC && result) {
        // We already copied; free the original
        Tcl_Free(const_cast<char *>(result));
    }
    // Clear obj result
    if (impl->resultObj) {
        Tcl_DecrRefCount(impl->resultObj);
        impl->resultObj = nullptr;
    }
}

const char *Tcl_GetStringResult(Tcl_Interp *interp) {
    auto *impl = minitcl::getImpl(interp);
    if (impl->resultObj && impl->resultObj->bytes) {
        return impl->resultObj->bytes;
    }
    return impl->result.c_str();
}

void Tcl_ResetResult(Tcl_Interp *interp) {
    auto *impl = minitcl::getImpl(interp);
    impl->result.clear();
    if (impl->resultObj) {
        Tcl_DecrRefCount(impl->resultObj);
        impl->resultObj = nullptr;
    }
}

void Tcl_AppendResult(Tcl_Interp *interp, ...) {
    auto *impl = minitcl::getImpl(interp);
    va_list args;
    va_start(args, interp);
    while (const char *s = va_arg(args, const char *)) {
        impl->result += s;
    }
    va_end(args);
}

void Tcl_SetObjResult(Tcl_Interp *interp, Tcl_Obj *resultObjPtr) {
    auto *impl = minitcl::getImpl(interp);
    if (impl->resultObj) {
        Tcl_DecrRefCount(impl->resultObj);
    }
    impl->resultObj = resultObjPtr;
    if (resultObjPtr) {
        Tcl_IncrRefCount(resultObjPtr);
        if (resultObjPtr->bytes) {
            impl->result = resultObjPtr->bytes;
        }
    }
}

Tcl_Obj *Tcl_GetObjResult(Tcl_Interp *interp) {
    auto *impl = minitcl::getImpl(interp);
    if (impl->resultObj) {
        return impl->resultObj;
    }
    // Create a string obj from the result
    Tcl_Obj *obj = Tcl_NewStringObj(impl->result.c_str(), impl->result.size());
    Tcl_SetObjResult(interp, obj);
    return obj;
}

Tcl_Obj *Tcl_GetReturnOptions(Tcl_Interp *, int) {
    // Return empty dict-like list
    return Tcl_NewListObj(0, nullptr);
}

// ============================================================
// Object creation and access
// ============================================================

static Tcl_Obj *newObj() {
    auto *obj = new Tcl_Obj();
    obj->refCount = 0;
    obj->bytes = nullptr;
    obj->length = 0;
    obj->internalRep = nullptr;
    obj->typePtr = nullptr;
    return obj;
}

static void setObjString(Tcl_Obj *obj, const char *bytes, int length) {
    delete[] obj->bytes;
    if (bytes) {
        if (length < 0) length = strlen(bytes);
        obj->bytes = new char[length + 1];
        memcpy(obj->bytes, bytes, length);
        obj->bytes[length] = '\0';
        obj->length = length;
    } else {
        obj->bytes = nullptr;
        obj->length = 0;
    }
}

Tcl_Obj *Tcl_NewObj(void) {
    Tcl_Obj *obj = newObj();
    setObjString(obj, "", 0);
    return obj;
}

Tcl_Obj *Tcl_NewStringObj(const char *bytes, int length) {
    Tcl_Obj *obj = newObj();
    if (bytes) {
        setObjString(obj, bytes, length);
    } else {
        setObjString(obj, "", 0);
    }
    return obj;
}

Tcl_Obj *Tcl_NewIntObj(int intValue) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", intValue);
    return Tcl_NewStringObj(buf, -1);
}

Tcl_Obj *Tcl_NewDoubleObj(double doubleValue) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", doubleValue);
    return Tcl_NewStringObj(buf, -1);
}

Tcl_Obj *Tcl_NewBooleanObj(int boolValue) {
    return Tcl_NewIntObj(boolValue ? 1 : 0);
}

Tcl_Obj *Tcl_NewWideIntObj(long long wideValue) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", wideValue);
    return Tcl_NewStringObj(buf, -1);
}

Tcl_Obj *Tcl_NewListObj(int objc, Tcl_Obj *const objv[]) {
    Tcl_Obj *list = newObj();
    // Store list elements in internalRep as a vector
    auto *elements = new std::vector<Tcl_Obj *>();
    if (objv) {
        for (int i = 0; i < objc; i++) {
            Tcl_IncrRefCount(objv[i]);
            elements->push_back(objv[i]);
        }
    }
    list->internalRep = elements;

    // Build string rep
    std::string rep;
    for (int i = 0; i < objc; i++) {
        if (i > 0) rep += ' ';
        const char *s = Tcl_GetString(objv[i]);
        // Simple quoting: brace if contains spaces
        bool needs_quoting = false;
        for (const char *p = s; *p; p++) {
            if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '{' ||
                *p == '}' || *p == '"' || *p == '\\') {
                needs_quoting = true;
                break;
            }
        }
        if (needs_quoting || *s == '\0') {
            rep += '{';
            rep += s;
            rep += '}';
        } else {
            rep += s;
        }
    }
    setObjString(list, rep.c_str(), rep.size());
    return list;
}

char *Tcl_GetString(Tcl_Obj *objPtr) {
    if (!objPtr) return const_cast<char *>("");
    if (!objPtr->bytes) {
        setObjString(objPtr, "", 0);
    }
    return objPtr->bytes;
}

char *Tcl_GetStringFromObj(Tcl_Obj *objPtr, int *lengthPtr) {
    if (!objPtr) {
        if (lengthPtr) *lengthPtr = 0;
        return "";
    }
    char *s = Tcl_GetString(objPtr);
    if (lengthPtr) *lengthPtr = objPtr->length;
    return s;
}

int Tcl_GetInt(Tcl_Interp *interp, const char *src, int *intPtr) {
    char *end;
    long val = strtol(src, &end, 0);
    if (end == src || *end != '\0') {
        if (interp) {
            auto *impl = minitcl::getImpl(interp);
            impl->result = std::string("expected integer but got \"") + src + "\"";
        }
        return TCL_ERROR;
    }
    *intPtr = static_cast<int>(val);
    return TCL_OK;
}

int Tcl_GetDouble(Tcl_Interp *interp, const char *src, double *doublePtr) {
    char *end;
    double val = strtod(src, &end);
    if (end == src || *end != '\0') {
        if (interp) {
            auto *impl = minitcl::getImpl(interp);
            impl->result =
                std::string("expected floating-point number but got \"") + src + "\"";
        }
        return TCL_ERROR;
    }
    *doublePtr = val;
    return TCL_OK;
}

int Tcl_GetDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                          double *doublePtr) {
    return Tcl_GetDouble(interp, Tcl_GetString(objPtr), doublePtr);
}

int Tcl_GetWideIntFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                           long long *widePtr) {
    char *end;
    const char *src = Tcl_GetString(objPtr);
    long long val = strtoll(src, &end, 0);
    if (end == src || *end != '\0') {
        if (interp) {
            auto *impl = minitcl::getImpl(interp);
            impl->result =
                std::string("expected integer but got \"") + src + "\"";
        }
        return TCL_ERROR;
    }
    *widePtr = val;
    return TCL_OK;
}

int Tcl_GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                           int *boolPtr) {
    const char *src = Tcl_GetString(objPtr);
    if (strcmp(src, "1") == 0 || strcmp(src, "true") == 0 ||
        strcmp(src, "yes") == 0 || strcmp(src, "on") == 0) {
        *boolPtr = 1;
        return TCL_OK;
    }
    if (strcmp(src, "0") == 0 || strcmp(src, "false") == 0 ||
        strcmp(src, "no") == 0 || strcmp(src, "off") == 0) {
        *boolPtr = 0;
        return TCL_OK;
    }
    if (interp) {
        auto *impl = minitcl::getImpl(interp);
        impl->result =
            std::string("expected boolean value but got \"") + src + "\"";
    }
    return TCL_ERROR;
}

void Tcl_SetIntObj(Tcl_Obj *objPtr, int intValue) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", intValue);
    setObjString(objPtr, buf, -1);
}

// ============================================================
// Reference counting
// ============================================================

void Tcl_IncrRefCount(Tcl_Obj *objPtr) {
    if (objPtr) objPtr->refCount++;
}

void Tcl_DecrRefCount(Tcl_Obj *objPtr) {
    if (!objPtr) return;
    if (--objPtr->refCount <= 0) {
        // Free list elements if any
        if (objPtr->internalRep) {
            auto *elements =
                static_cast<std::vector<Tcl_Obj *> *>(objPtr->internalRep);
            for (auto *elem : *elements) {
                Tcl_DecrRefCount(elem);
            }
            delete elements;
        }
        delete[] objPtr->bytes;
        delete objPtr;
    }
}

// ============================================================
// List operations
// ============================================================

int Tcl_ListObjGetElements(Tcl_Interp *, Tcl_Obj *listPtr,
                            int *objcPtr, Tcl_Obj ***objvPtr) {
    if (!listPtr) {
        *objcPtr = 0;
        *objvPtr = nullptr;
        return TCL_OK;
    }
    if (listPtr->internalRep) {
        auto *elements =
            static_cast<std::vector<Tcl_Obj *> *>(listPtr->internalRep);
        *objcPtr = static_cast<int>(elements->size());
        *objvPtr = elements->empty() ? nullptr : elements->data();
        return TCL_OK;
    }
    // Parse string rep into list elements
    if (listPtr->bytes && listPtr->bytes[0] != '\0') {
        auto parsed = minitcl::parseScript(listPtr->bytes);
        auto *elements = new std::vector<Tcl_Obj *>();
        for (const auto &cmd : parsed) {
            for (const auto &word : cmd.words) {
                Tcl_Obj *obj = Tcl_NewStringObj(word.text.c_str(),
                                                 word.text.size());
                Tcl_IncrRefCount(obj);
                elements->push_back(obj);
            }
        }
        listPtr->internalRep = elements;
        *objcPtr = static_cast<int>(elements->size());
        *objvPtr = elements->empty() ? nullptr : elements->data();
        return TCL_OK;
    }
    *objcPtr = 0;
    *objvPtr = nullptr;
    return TCL_OK;
}

int Tcl_ListObjAppendElement(Tcl_Interp *, Tcl_Obj *listPtr,
                              Tcl_Obj *objPtr) {
    if (!listPtr) return TCL_ERROR;
    if (!listPtr->internalRep) {
        listPtr->internalRep = new std::vector<Tcl_Obj *>();
    }
    auto *elements =
        static_cast<std::vector<Tcl_Obj *> *>(listPtr->internalRep);
    Tcl_IncrRefCount(objPtr);
    elements->push_back(objPtr);

    // Update string rep
    std::string rep;
    for (size_t i = 0; i < elements->size(); i++) {
        if (i > 0) rep += ' ';
        const char *s = Tcl_GetString((*elements)[i]);
        bool needs_quoting = false;
        for (const char *p = s; *p; p++) {
            if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '{' ||
                *p == '}' || *p == '"' || *p == '\\') {
                needs_quoting = true;
                break;
            }
        }
        if (needs_quoting || *s == '\0') {
            rep += '{';
            rep += s;
            rep += '}';
        } else {
            rep += s;
        }
    }
    setObjString(listPtr, rep.c_str(), rep.size());
    return TCL_OK;
}

int Tcl_ListObjLength(Tcl_Interp *, Tcl_Obj *listPtr, int *lengthPtr) {
    if (!listPtr || !listPtr->internalRep) {
        *lengthPtr = 0;
        return TCL_OK;
    }
    auto *elements =
        static_cast<std::vector<Tcl_Obj *> *>(listPtr->internalRep);
    *lengthPtr = static_cast<int>(elements->size());
    return TCL_OK;
}

int Tcl_ListObjIndex(Tcl_Interp *, Tcl_Obj *listPtr, int index,
                      Tcl_Obj **objPtrPtr) {
    if (!listPtr || !listPtr->internalRep) {
        *objPtrPtr = nullptr;
        return TCL_OK;
    }
    auto *elements =
        static_cast<std::vector<Tcl_Obj *> *>(listPtr->internalRep);
    if (index < 0 || index >= static_cast<int>(elements->size())) {
        *objPtrPtr = nullptr;
        return TCL_OK;
    }
    *objPtrPtr = (*elements)[index];
    return TCL_OK;
}

// ============================================================
// Command registration (stubs - functional in Phase 3)
// ============================================================

Tcl_Command Tcl_CreateObjCommand(Tcl_Interp *interp, const char *cmdName,
                                  Tcl_ObjCmdProc *proc,
                                  ClientData clientData,
                                  Tcl_CmdDeleteProc *deleteProc) {
    auto *impl = minitcl::getImpl(interp);
    impl->commands[cmdName] = {proc, nullptr, clientData, deleteProc};
    // Return a non-null token
    return reinterpret_cast<Tcl_Command>(1);
}

Tcl_Command Tcl_CreateCommand(Tcl_Interp *interp, const char *cmdName,
                               Tcl_CmdProc *proc,
                               ClientData clientData,
                               Tcl_CmdDeleteProc *deleteProc) {
    auto *impl = minitcl::getImpl(interp);
    impl->commands[cmdName] = {nullptr, proc, clientData, deleteProc};
    return reinterpret_cast<Tcl_Command>(1);
}

// ============================================================
// Variable scoping implementation
// ============================================================

namespace minitcl {

const char *InterpImpl::getVar(const std::string &name) const {
    // Check if it's a global reference (::var)
    if (name.size() > 2 && name[0] == ':' && name[1] == ':') {
        std::string globalName = name.substr(2);
        auto it = globals.find(globalName);
        return it != globals.end() ? it->second.c_str() : nullptr;
    }

    // If we have a call frame, check locals first
    if (!callStack.empty()) {
        const auto &frame = callStack.back();

        // Check for upvar link (exact name or array base name)
        auto linkIt = frame.upvarLinks.find(name);
        if (linkIt != frame.upvarLinks.end()) {
            int targetFrame = linkIt->second.first;
            const std::string &targetName = linkIt->second.second;
            return getVarInFrame(targetFrame, targetName);
        }
        // Array access: check if base name has an upvar link
        auto paren = name.find('(');
        if (paren != std::string::npos) {
            std::string baseName = name.substr(0, paren);
            linkIt = frame.upvarLinks.find(baseName);
            if (linkIt != frame.upvarLinks.end()) {
                int targetFrame = linkIt->second.first;
                std::string targetName = linkIt->second.second + name.substr(paren);
                return getVarInFrame(targetFrame, targetName);
            }
        }

        auto it = frame.locals.find(name);
        if (it != frame.locals.end()) return it->second.c_str();
        return nullptr;  // In a proc, don't fall through to globals
    }

    // Global scope
    auto it = globals.find(name);
    return it != globals.end() ? it->second.c_str() : nullptr;
}

void InterpImpl::setVar(const std::string &name, const std::string &value) {
    // Global reference (::var)
    if (name.size() > 2 && name[0] == ':' && name[1] == ':') {
        globals[name.substr(2)] = value;
        return;
    }

    if (!callStack.empty()) {
        auto &frame = callStack.back();

        // Check for upvar link (exact name or array base name)
        auto linkIt = frame.upvarLinks.find(name);
        if (linkIt != frame.upvarLinks.end()) {
            int targetFrame = linkIt->second.first;
            const std::string &targetName = linkIt->second.second;
            setVarInFrame(targetFrame, targetName, value);
            return;
        }
        // Array access: check if base name has an upvar link
        auto paren = name.find('(');
        if (paren != std::string::npos) {
            std::string baseName = name.substr(0, paren);
            linkIt = frame.upvarLinks.find(baseName);
            if (linkIt != frame.upvarLinks.end()) {
                int targetFrame = linkIt->second.first;
                std::string targetName = linkIt->second.second + name.substr(paren);
                setVarInFrame(targetFrame, targetName, value);
                return;
            }
        }

        frame.locals[name] = value;
        return;
    }

    globals[name] = value;
}

bool InterpImpl::unsetVar(const std::string &name) {
    if (name.size() > 2 && name[0] == ':' && name[1] == ':') {
        return globals.erase(name.substr(2)) > 0;
    }
    if (!callStack.empty()) {
        return callStack.back().locals.erase(name) > 0;
    }
    return globals.erase(name) > 0;
}

bool InterpImpl::varExists(const std::string &name) const {
    return getVar(name) != nullptr;
}

const char *InterpImpl::getVarInFrame(int frameIdx,
                                       const std::string &name) const {
    if (frameIdx < 0) {
        // Global scope
        auto it = globals.find(name);
        return it != globals.end() ? it->second.c_str() : nullptr;
    }
    if (frameIdx < static_cast<int>(callStack.size())) {
        const auto &frame = callStack[frameIdx];
        // Follow upvar chains in the target frame
        auto linkIt = frame.upvarLinks.find(name);
        if (linkIt != frame.upvarLinks.end()) {
            return getVarInFrame(linkIt->second.first, linkIt->second.second);
        }
        // Also check array base name for upvar
        auto paren = name.find('(');
        if (paren != std::string::npos) {
            std::string baseName = name.substr(0, paren);
            linkIt = frame.upvarLinks.find(baseName);
            if (linkIt != frame.upvarLinks.end()) {
                std::string targetName = linkIt->second.second + name.substr(paren);
                return getVarInFrame(linkIt->second.first, targetName);
            }
        }
        auto it = frame.locals.find(name);
        return it != frame.locals.end() ? it->second.c_str() : nullptr;
    }
    return nullptr;
}

void InterpImpl::setVarInFrame(int frameIdx, const std::string &name,
                                const std::string &value) {
    if (frameIdx < 0) {
        globals[name] = value;
        return;
    }
    if (frameIdx < static_cast<int>(callStack.size())) {
        auto &frame = callStack[frameIdx];
        // Follow upvar chains
        auto linkIt = frame.upvarLinks.find(name);
        if (linkIt != frame.upvarLinks.end()) {
            setVarInFrame(linkIt->second.first, linkIt->second.second, value);
            return;
        }
        auto paren = name.find('(');
        if (paren != std::string::npos) {
            std::string baseName = name.substr(0, paren);
            linkIt = frame.upvarLinks.find(baseName);
            if (linkIt != frame.upvarLinks.end()) {
                std::string targetName = linkIt->second.second + name.substr(paren);
                setVarInFrame(linkIt->second.first, targetName, value);
                return;
            }
        }
        frame.locals[name] = value;
    }
}

}  // namespace minitcl

// ============================================================
// Variables (C API)
// ============================================================

const char *Tcl_SetVar(Tcl_Interp *interp, const char *varName,
                        const char *newValue, int flags) {
    auto *impl = minitcl::getImpl(interp);
    if (flags & TCL_GLOBAL_ONLY) {
        // Strip :: prefix for global storage
        std::string name = varName;
        if (name.size() > 2 && name[0] == ':' && name[1] == ':') {
            name = name.substr(2);
        }
        impl->globals[name] = newValue ? newValue : "";
        return impl->globals[name].c_str();
    }
    impl->setVar(varName, newValue ? newValue : "");
    return impl->getVar(varName);
}

const char *Tcl_GetVar(Tcl_Interp *interp, const char *varName, int flags) {
    auto *impl = minitcl::getImpl(interp);
    if (flags & TCL_GLOBAL_ONLY) {
        std::string name = varName;
        if (name.size() > 2 && name[0] == ':' && name[1] == ':') {
            name = name.substr(2);
        }
        auto it = impl->globals.find(name);
        return it != impl->globals.end() ? it->second.c_str() : nullptr;
    }
    return impl->getVar(varName);
}

const char *Tcl_GetVar2(Tcl_Interp *interp, const char *part1,
                          const char *part2, int flags) {
    if (!part2) return Tcl_GetVar(interp, part1, flags);
    std::string name = std::string(part1) + "(" + part2 + ")";
    return Tcl_GetVar(interp, name.c_str(), flags);
}

Tcl_Obj *Tcl_SetVar2Ex(Tcl_Interp *interp, const char *part1,
                         const char *part2, Tcl_Obj *newValuePtr, int flags) {
    const char *value = Tcl_GetString(newValuePtr);
    std::string name = part1;
    if (part2) {
        name += "(";
        name += part2;
        name += ")";
    }
    Tcl_SetVar(interp, name.c_str(), value, flags);
    return newValuePtr;
}

// ============================================================
// Associated data
// ============================================================

void Tcl_SetAssocData(Tcl_Interp *interp, const char *name,
                       Tcl_InterpDeleteProc *proc, ClientData clientData) {
    auto *impl = minitcl::getImpl(interp);
    impl->assocData[name] = {proc, clientData};
}

ClientData Tcl_GetAssocData(Tcl_Interp *interp, const char *name,
                             Tcl_InterpDeleteProc **procPtr) {
    auto *impl = minitcl::getImpl(interp);
    auto it = impl->assocData.find(name);
    if (it == impl->assocData.end()) {
        if (procPtr) *procPtr = nullptr;
        return nullptr;
    }
    if (procPtr) *procPtr = it->second.first;
    return it->second.second;
}

// ============================================================
// Static packages (no-op)
// ============================================================

void Tcl_StaticPackage(Tcl_Interp *, const char *, Tcl_AppInitProc *,
                        Tcl_AppInitProc *) {
    // No-op
}

// Regexp C API now in regexp.cc

// Channel C API now in channel.cc

// ============================================================
// DString
// ============================================================

void Tcl_DStringInit(Tcl_DString *dsPtr) {
    dsPtr->string = dsPtr->staticSpace;
    dsPtr->length = 0;
    dsPtr->spaceAvl = sizeof(dsPtr->staticSpace);
    dsPtr->staticSpace[0] = '\0';
}

char *Tcl_DStringAppend(Tcl_DString *dsPtr, const char *bytes, int length) {
    if (length < 0) length = strlen(bytes);
    int newLength = dsPtr->length + length;
    if (newLength + 1 > dsPtr->spaceAvl) {
        int newSpace = newLength * 2 + 1;
        char *newStr = new char[newSpace];
        memcpy(newStr, dsPtr->string, dsPtr->length);
        if (dsPtr->string != dsPtr->staticSpace) {
            delete[] dsPtr->string;
        }
        dsPtr->string = newStr;
        dsPtr->spaceAvl = newSpace;
    }
    memcpy(dsPtr->string + dsPtr->length, bytes, length);
    dsPtr->length = newLength;
    dsPtr->string[newLength] = '\0';
    return dsPtr->string;
}

void Tcl_DStringFree(Tcl_DString *dsPtr) {
    if (dsPtr->string != dsPtr->staticSpace) {
        delete[] dsPtr->string;
    }
    dsPtr->string = dsPtr->staticSpace;
    dsPtr->length = 0;
    dsPtr->spaceAvl = sizeof(dsPtr->staticSpace);
    dsPtr->staticSpace[0] = '\0';
}

char *Tcl_DStringValue(Tcl_DString *dsPtr) {
    return dsPtr->string;
}

int Tcl_DStringLength(Tcl_DString *dsPtr) {
    return dsPtr->length;
}

// ============================================================
// Tcl_Free (needed for TCL_DYNAMIC)
// ============================================================

void Tcl_Free(char *ptr) {
    free(ptr);
}

char *Tcl_Alloc(unsigned int size) {
    return static_cast<char *>(malloc(size));
}

// Glob-style pattern matching (supports *, ?, [chars], \escape)
int Tcl_StringMatch(const char *str, const char *pattern) {
    while (*pattern) {
        if (*pattern == '*') {
            pattern++;
            if (!*pattern) return 1;  // trailing * matches everything
            while (*str) {
                if (Tcl_StringMatch(str, pattern)) return 1;
                str++;
            }
            return 0;
        } else if (*pattern == '?') {
            if (!*str) return 0;
            str++;
            pattern++;
        } else if (*pattern == '[') {
            pattern++;
            bool invert = (*pattern == '^');
            if (invert) pattern++;
            bool matched = false;
            while (*pattern && *pattern != ']') {
                if (*(pattern + 1) == '-' && *(pattern + 2) && *(pattern + 2) != ']') {
                    if (*str >= *pattern && *str <= *(pattern + 2)) matched = true;
                    pattern += 3;
                } else {
                    if (*str == *pattern) matched = true;
                    pattern++;
                }
            }
            if (*pattern == ']') pattern++;
            if (invert) matched = !matched;
            if (!matched) return 0;
            str++;
        } else if (*pattern == '\\') {
            pattern++;
            if (*pattern != *str) return 0;
            if (*pattern) pattern++;
            if (*str) str++;
        } else {
            if (*pattern != *str) return 0;
            pattern++;
            str++;
        }
    }
    return *str == '\0';
}
