// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Stub implementations for SWIG runtime functions

#include "tcl.h"
#include "interp.h"

#include <cstdarg>
#include <cstring>
#include <map>
#include <string>

// ============================================================
// SWIG runtime stubs
// ============================================================

int Tcl_GetCommandInfo(Tcl_Interp *interp, const char *cmdName,
                        Tcl_CmdInfo *infoPtr) {
    auto *impl = minitcl::getImpl(interp);
    auto it = impl->commands.find(cmdName);
    if (it == impl->commands.end()) return 0;
    if (infoPtr) {
        memset(infoPtr, 0, sizeof(*infoPtr));
        infoPtr->objProc = it->second.objProc;
        infoPtr->objClientData = it->second.clientData;
        infoPtr->proc = it->second.stringProc;
        infoPtr->clientData = it->second.clientData;
        infoPtr->deleteProc = it->second.deleteProc;
        infoPtr->isNativeObjectProc = (it->second.objProc != nullptr);
    }
    return 1;
}

int Tcl_SetCommandInfo(Tcl_Interp *, const char *, const Tcl_CmdInfo *) {
    return 1;
}

void Tcl_DeleteCommandFromToken(Tcl_Interp *, Tcl_Command) {
    // No-op: commands are cleaned up on interp delete
}

Tcl_Obj *Tcl_DuplicateObj(Tcl_Obj *objPtr) {
    if (!objPtr) return Tcl_NewObj();
    return Tcl_NewStringObj(Tcl_GetString(objPtr), objPtr->length);
}

Tcl_Obj *Tcl_ObjSetVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
                          Tcl_Obj *part2Ptr, Tcl_Obj *newValuePtr,
                          int flags) {
    return Tcl_SetVar2Ex(interp, Tcl_GetString(part1Ptr),
                          part2Ptr ? Tcl_GetString(part2Ptr) : nullptr,
                          newValuePtr, flags);
}

void Tcl_AppendElement(Tcl_Interp *interp, const char *element) {
    auto *impl = minitcl::getImpl(interp);
    if (!impl->result.empty()) impl->result += ' ';
    // Simple quoting
    bool needsQuoting = false;
    for (const char *p = element; *p; p++) {
        if (*p == ' ' || *p == '\t' || *p == '\n') {
            needsQuoting = true;
            break;
        }
    }
    if (needsQuoting) {
        impl->result += '{';
        impl->result += element;
        impl->result += '}';
    } else {
        impl->result += element;
    }
}

int Tcl_VarEval(Tcl_Interp *interp, ...) {
    std::string script;
    va_list args;
    va_start(args, interp);
    while (const char *s = va_arg(args, const char *)) {
        script += s;
    }
    va_end(args);
    return Tcl_Eval(interp, script.c_str());
}

void Tcl_AddErrorInfo(Tcl_Interp *interp, const char *message) {
    const char *cur = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    std::string info = cur ? cur : "";
    info += message;
    Tcl_SetVar(interp, "errorInfo", info.c_str(), TCL_GLOBAL_ONLY);
}

void Tcl_SetErrorCode(Tcl_Interp *interp, ...) {
    std::string code;
    va_list args;
    va_start(args, interp);
    while (const char *s = va_arg(args, const char *)) {
        if (!code.empty()) code += ' ';
        code += s;
    }
    va_end(args);
    Tcl_SetVar(interp, "errorCode", code.c_str(), TCL_GLOBAL_ONLY);
}

// ============================================================
// Hash table stubs (minimal - SWIG uses these for type tracking)
// ============================================================

// Hash table using a map of heap-allocated entries for stable pointers
struct HashTableImpl {
    std::map<std::string, Tcl_HashEntry *> entries;
    ~HashTableImpl() {
        for (auto &[k, e] : entries) delete e;
    }
};

void Tcl_InitHashTable(Tcl_HashTable *tablePtr, int) {
    auto *impl = new HashTableImpl();
    tablePtr->buckets = reinterpret_cast<Tcl_HashEntry **>(impl);
    tablePtr->numEntries = 0;
}

void Tcl_DeleteHashTable(Tcl_HashTable *tablePtr) {
    auto *impl = reinterpret_cast<HashTableImpl *>(tablePtr->buckets);
    delete impl;
    tablePtr->buckets = nullptr;
    tablePtr->numEntries = 0;
}

Tcl_HashEntry *Tcl_CreateHashEntry(Tcl_HashTable *tablePtr,
                                    const char *key, int *newPtr) {
    auto *impl = reinterpret_cast<HashTableImpl *>(tablePtr->buckets);
    auto it = impl->entries.find(key);
    if (it != impl->entries.end()) {
        if (newPtr) *newPtr = 0;
        return it->second;
    }
    if (newPtr) *newPtr = 1;
    auto *entry = new Tcl_HashEntry();
    entry->key = key;
    entry->clientData = nullptr;
    impl->entries[key] = entry;
    tablePtr->numEntries = impl->entries.size();
    return entry;
}

Tcl_HashEntry *Tcl_FindHashEntry(Tcl_HashTable *tablePtr, const char *key) {
    auto *impl = reinterpret_cast<HashTableImpl *>(tablePtr->buckets);
    auto it = impl->entries.find(key);
    if (it == impl->entries.end()) return nullptr;
    return it->second;
}

void Tcl_DeleteHashEntry(Tcl_HashEntry *) {
    // Simplified: entries are removed on table delete
}

void Tcl_SetHashValue(Tcl_HashEntry *entryPtr, ClientData value) {
    if (entryPtr) entryPtr->clientData = value;
}

ClientData Tcl_GetHashValue(Tcl_HashEntry *entryPtr) {
    return entryPtr ? entryPtr->clientData : nullptr;
}

const char *Tcl_GetHashKey(Tcl_HashTable *, Tcl_HashEntry *entryPtr) {
    return entryPtr ? entryPtr->key : "";
}

Tcl_HashEntry *Tcl_FirstHashEntry(Tcl_HashTable *, Tcl_HashSearch *) {
    return nullptr;  // Iteration not needed by SWIG
}

Tcl_HashEntry *Tcl_NextHashEntry(Tcl_HashSearch *) {
    return nullptr;
}

// ============================================================
// Additional SWIG-needed stubs
// ============================================================

Tcl_Obj *Tcl_ObjGetVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
                         Tcl_Obj *part2Ptr, int flags) {
    const char *val = Tcl_GetVar2(interp, Tcl_GetString(part1Ptr),
                                   part2Ptr ? Tcl_GetString(part2Ptr) : nullptr,
                                   flags);
    if (!val) return nullptr;
    return Tcl_NewStringObj(val, -1);
}

int Tcl_GetIntFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *intPtr) {
    return Tcl_GetInt(interp, Tcl_GetString(objPtr), intPtr);
}

const char *Tcl_SetVar2(Tcl_Interp *interp, const char *part1,
                          const char *part2, const char *newValue, int flags) {
    std::string name = part1;
    if (part2) {
        name += "(";
        name += part2;
        name += ")";
    }
    return Tcl_SetVar(interp, name.c_str(), newValue, flags);
}

int Tcl_PkgProvide(Tcl_Interp *, const char *, const char *) {
    return TCL_OK;
}

Tcl_Obj *Tcl_NewLongObj(long longValue) {
    return Tcl_NewWideIntObj(longValue);
}

int Tcl_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, long *longPtr) {
    long long wide;
    int rc = Tcl_GetWideIntFromObj(interp, objPtr, &wide);
    if (rc == TCL_OK && longPtr) *longPtr = static_cast<long>(wide);
    return rc;
}

int Tcl_TraceVar(Tcl_Interp *, const char *, int,
                  Tcl_VarTraceProc *, ClientData) {
    return TCL_OK;  // No-op stub
}

void Tcl_UntraceVar(Tcl_Interp *, const char *, int,
                     Tcl_VarTraceProc *, ClientData) {
    // No-op stub
}
