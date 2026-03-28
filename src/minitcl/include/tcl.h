// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Minimal Tcl interpreter for OpenROAD
// Drop-in replacement for tcl.h

#pragma once

#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Version info - we claim Tcl 9 so that version-gated code
// takes the modern path (no encapCloseProc, CONST84 = const, etc.)
#define TCL_MAJOR_VERSION 9
#define TCL_MINOR_VERSION 0
#define TCL_VERSION "9.0"
#define TCL_PATCH_LEVEL "9.0.0"

// Return codes
#define TCL_OK       0
#define TCL_ERROR    1
#define TCL_RETURN   2
#define TCL_BREAK    3
#define TCL_CONTINUE 4

// Tcl_SetResult freeProc values
#define TCL_STATIC   ((Tcl_FreeProc*) 0)
#define TCL_VOLATILE ((Tcl_FreeProc*) 1)
#define TCL_DYNAMIC  ((Tcl_FreeProc*) 3)

// Compatibility macros
#define CONST84 const

// Channel constants
#define TCL_STDIN            (1 << 1)
#define TCL_STDOUT           (1 << 2)
#define TCL_STDERR           (1 << 3)
#define TCL_READABLE         (1 << 1)
#define TCL_WRITABLE         (1 << 2)
#define TCL_CHANNEL_VERSION_5 ((Tcl_ChannelTypeVersion) 0x5)

// Regexp flags
#define TCL_REG_ADVANCED     0x01
#define TCL_REG_NOCASE       0x02

// Variable flags
#define TCL_GLOBAL_ONLY      0x01
#define TCL_NAMESPACE_ONLY   0x02
#define TCL_LEAVE_ERR_MSG    0x04
#define TCL_APPEND_VALUE     0x08
#define TCL_LIST_ELEMENT     0x10

// Trace flags
#define TCL_TRACE_READS      0x10
#define TCL_TRACE_WRITES     0x20
#define TCL_TRACE_UNSETS     0x40

// Eval flags
#define TCL_EVAL_GLOBAL      0x01
#define TCL_EVAL_DIRECT      0x02

// Tcl_Size - integer type for sizes
typedef int Tcl_Size;

// Tcl_WideInt - wide integer type
typedef long long Tcl_WideInt;
typedef unsigned long long Tcl_WideUInt;

// ClientData - opaque pointer for user data
typedef void* ClientData;

// Forward declarations
typedef struct Tcl_Interp Tcl_Interp;
typedef struct Tcl_Obj Tcl_Obj;
typedef struct Tcl_RegExp_ *Tcl_RegExp;
typedef void *Tcl_Channel;
typedef void *Tcl_ChannelTypeVersion;
typedef struct Tcl_DString Tcl_DString;

// Free proc type
typedef void (Tcl_FreeProc)(char *blockPtr);

// Function pointer types
typedef int (Tcl_ObjCmdProc)(ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *const objv[]);
typedef void (Tcl_CmdDeleteProc)(ClientData clientData);
typedef int (Tcl_CmdProc)(ClientData clientData, Tcl_Interp *interp,
                           int argc, const char *argv[]);
typedef void (Tcl_FreeInternalRepProc)(Tcl_Obj *objPtr);
typedef void (Tcl_DupInternalRepProc)(Tcl_Obj *srcPtr, Tcl_Obj *dupPtr);
typedef void (Tcl_UpdateStringProc)(Tcl_Obj *objPtr);
typedef int (Tcl_SetFromAnyProc)(Tcl_Interp *interp, Tcl_Obj *objPtr);

// App init callback for Tcl_Main
typedef int (Tcl_AppInitProc)(Tcl_Interp *interp);

// Channel driver procs
typedef int (Tcl_DriverOutputProc)(ClientData instanceData,
                                    const char *buf, int toWrite,
                                    int *errorCodePtr);
typedef int (Tcl_DriverInputProc)(ClientData instanceData,
                                   char *buf, int bufSize,
                                   int *errorCodePtr);
typedef int (Tcl_DriverCloseProc)(ClientData instanceData,
                                   Tcl_Interp *interp);
typedef int (Tcl_DriverSetOptionProc)(ClientData instanceData,
                                      Tcl_Interp *interp,
                                      const char *optionName,
                                      const char *value);
typedef int (Tcl_DriverGetOptionProc)(ClientData instanceData,
                                      Tcl_Interp *interp,
                                      const char *optionName,
                                      Tcl_DString *dsPtr);
typedef void (Tcl_DriverWatchProc)(ClientData instanceData, int mask);
typedef int (Tcl_DriverGetHandleProc)(ClientData instanceData,
                                      int direction,
                                      ClientData *handlePtr);
typedef int (Tcl_DriverBlockModeProc)(ClientData instanceData, int mode);

// Tcl_ChannelType - Tcl 9 layout
typedef struct Tcl_ChannelType {
    const char *typeName;
    Tcl_ChannelTypeVersion version;
    void *closeProc;  // unused in Tcl 9
    Tcl_DriverInputProc *inputProc;
    Tcl_DriverOutputProc *outputProc;
    void *close2Proc;
    Tcl_DriverSetOptionProc *setOptionProc;
    Tcl_DriverGetOptionProc *getOptionProc;
    Tcl_DriverWatchProc *watchProc;
    Tcl_DriverGetHandleProc *getHandleProc;
    void *close2Proc2;
    Tcl_DriverBlockModeProc *blockModeProc;
    void *flushProc;
    void *handlerProc;
    void *wideSeekProc;
    void *threadActionProc;
    void *truncateProc;
} Tcl_ChannelType;

// Tcl_Obj - the core value type
struct Tcl_Obj {
    int refCount;
    char *bytes;
    int length;
    // Internal representation - managed by minitcl
    void *internalRep;
    void *typePtr;
};

// Tcl_DString
struct Tcl_DString {
    char *string;
    int length;
    int spaceAvl;
    char staticSpace[200];
};

// ============================================================
// Interpreter lifecycle
// ============================================================
Tcl_Interp* Tcl_CreateInterp(void);
void        Tcl_DeleteInterp(Tcl_Interp *interp);
int         Tcl_Init(Tcl_Interp *interp);
void        Tcl_Main(int argc, char **argv, Tcl_AppInitProc *appInitProc);

// ============================================================
// Evaluation
// ============================================================
int Tcl_Eval(Tcl_Interp *interp, const char *script);
int Tcl_EvalFile(Tcl_Interp *interp, const char *fileName);
int Tcl_EvalObjEx(Tcl_Interp *interp, Tcl_Obj *objPtr, int flags);

// ============================================================
// Result handling
// ============================================================
void        Tcl_SetResult(Tcl_Interp *interp, const char *result,
                           Tcl_FreeProc *freeProc);
const char* Tcl_GetStringResult(Tcl_Interp *interp);
void        Tcl_ResetResult(Tcl_Interp *interp);
void        Tcl_AppendResult(Tcl_Interp *interp, ...);
void        Tcl_SetObjResult(Tcl_Interp *interp, Tcl_Obj *resultObjPtr);
Tcl_Obj*    Tcl_GetObjResult(Tcl_Interp *interp);
Tcl_Obj*    Tcl_GetReturnOptions(Tcl_Interp *interp, int code);

// ============================================================
// Object creation and access
// ============================================================
Tcl_Obj*    Tcl_NewObj(void);
Tcl_Obj*    Tcl_NewStringObj(const char *bytes, int length);
Tcl_Obj*    Tcl_NewIntObj(int intValue);
Tcl_Obj*    Tcl_NewDoubleObj(double doubleValue);
Tcl_Obj*    Tcl_NewBooleanObj(int boolValue);
Tcl_Obj*    Tcl_NewWideIntObj(long long wideValue);
Tcl_Obj*    Tcl_NewListObj(int objc, Tcl_Obj *const objv[]);

char*       Tcl_GetString(Tcl_Obj *objPtr);
char*       Tcl_GetStringFromObj(Tcl_Obj *objPtr, int *lengthPtr);
int         Tcl_GetInt(Tcl_Interp *interp, const char *src, int *intPtr);
int         Tcl_GetDouble(Tcl_Interp *interp, const char *src,
                           double *doublePtr);
int         Tcl_GetDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                                  double *doublePtr);
int         Tcl_GetIntFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                              int *intPtr);
int         Tcl_GetWideIntFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                                   long long *widePtr);
int         Tcl_GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                                   int *boolPtr);

void        Tcl_SetIntObj(Tcl_Obj *objPtr, int intValue);

// ============================================================
// Reference counting
// ============================================================
void Tcl_IncrRefCount(Tcl_Obj *objPtr);
void Tcl_DecrRefCount(Tcl_Obj *objPtr);

// Convenience macros (standard Tcl provides these)
#ifndef Tcl_IncrRefCount
// Already declared as functions above; some code uses the macro form
#endif

// ============================================================
// List operations
// ============================================================
int Tcl_ListObjGetElements(Tcl_Interp *interp, Tcl_Obj *listPtr,
                            int *objcPtr, Tcl_Obj ***objvPtr);
int Tcl_ListObjAppendElement(Tcl_Interp *interp, Tcl_Obj *listPtr,
                              Tcl_Obj *objPtr);
int Tcl_ListObjLength(Tcl_Interp *interp, Tcl_Obj *listPtr, int *lengthPtr);
int Tcl_ListObjIndex(Tcl_Interp *interp, Tcl_Obj *listPtr, int index,
                      Tcl_Obj **objPtrPtr);

// ============================================================
// Command registration
// ============================================================
typedef struct Tcl_Command_ *Tcl_Command;

Tcl_Command Tcl_CreateObjCommand(Tcl_Interp *interp, const char *cmdName,
                                  Tcl_ObjCmdProc *proc,
                                  ClientData clientData,
                                  Tcl_CmdDeleteProc *deleteProc);
Tcl_Command Tcl_CreateCommand(Tcl_Interp *interp, const char *cmdName,
                               Tcl_CmdProc *proc,
                               ClientData clientData,
                               Tcl_CmdDeleteProc *deleteProc);

// ============================================================
// Variables
// ============================================================
const char* Tcl_SetVar(Tcl_Interp *interp, const char *varName,
                        const char *newValue, int flags);
const char* Tcl_GetVar(Tcl_Interp *interp, const char *varName, int flags);
const char* Tcl_GetVar2(Tcl_Interp *interp, const char *part1,
                          const char *part2, int flags);
const char* Tcl_SetVar2(Tcl_Interp *interp, const char *part1,
                        const char *part2, const char *newValue, int flags);
Tcl_Obj*    Tcl_ObjGetVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
                           Tcl_Obj *part2Ptr, int flags);
Tcl_Obj*    Tcl_SetVar2Ex(Tcl_Interp *interp, const char *part1,
                           const char *part2, Tcl_Obj *newValuePtr,
                           int flags);

// ============================================================
// Associated data
// ============================================================
typedef void (Tcl_InterpDeleteProc)(ClientData clientData,
                                     Tcl_Interp *interp);

void        Tcl_SetAssocData(Tcl_Interp *interp, const char *name,
                              Tcl_InterpDeleteProc *proc,
                              ClientData clientData);
ClientData  Tcl_GetAssocData(Tcl_Interp *interp, const char *name,
                              Tcl_InterpDeleteProc **procPtr);

// ============================================================
// Static packages
// ============================================================
void Tcl_StaticPackage(Tcl_Interp *interp, const char *prefix,
                        Tcl_AppInitProc *initProc,
                        Tcl_AppInitProc *safeInitProc);

// ============================================================
// Regular expressions
// ============================================================
Tcl_RegExp  Tcl_GetRegExpFromObj(Tcl_Interp *interp, Tcl_Obj *patObj,
                                  int flags);
int         Tcl_RegExpExec(Tcl_Interp *interp, Tcl_RegExp regexp,
                            const char *text, const char *start);

// ============================================================
// Channels
// ============================================================
Tcl_Channel Tcl_GetStdChannel(int type);
Tcl_Channel Tcl_StackChannel(Tcl_Interp *interp,
                              const Tcl_ChannelType *typePtr,
                              ClientData instanceData, int mask,
                              Tcl_Channel prevChan);
int         Tcl_UnstackChannel(Tcl_Interp *interp, Tcl_Channel chan);
int         Tcl_Flush(Tcl_Channel chan);
const Tcl_ChannelType* Tcl_GetChannelType(Tcl_Channel chan);
ClientData  Tcl_GetChannelInstanceData(Tcl_Channel chan);
Tcl_DriverOutputProc* Tcl_ChannelOutputProc(const Tcl_ChannelType *chanTypePtr);

// Hash table types (used by SWIG runtime)
#define TCL_ONE_WORD_KEYS 0
#define TCL_STRING_KEYS   1

typedef struct Tcl_HashEntry {
    struct Tcl_HashEntry *nextPtr;
    void *clientData;
    const char *key;
} Tcl_HashEntry;

#define TCL_SMALL_HASH_TABLE 4

typedef struct Tcl_HashTable {
    Tcl_HashEntry **buckets;
    Tcl_HashEntry *staticBuckets[TCL_SMALL_HASH_TABLE];
    int numBuckets;
    int numEntries;
    int rebuildSize;
    int downShift;
    int mask;
    int keyType;
    void *findProc;
    void *createProc;
    struct Tcl_HashTable *nextPtr;
} Tcl_HashTable;

typedef struct Tcl_HashSearch {
    int dummy;
} Tcl_HashSearch;

// CmdInfo (used by SWIG runtime)
typedef struct Tcl_CmdInfo {
    int isNativeObjectProc;
    Tcl_ObjCmdProc *objProc;
    ClientData objClientData;
    Tcl_CmdProc *proc;
    ClientData clientData;
    Tcl_CmdDeleteProc *deleteProc;
    ClientData deleteData;
    void *namespacePtr;
} Tcl_CmdInfo;

// ============================================================
// Additional functions used by SWIG runtime
// ============================================================
int  Tcl_GetCommandInfo(Tcl_Interp *interp, const char *cmdName,
                         Tcl_CmdInfo *infoPtr);
int  Tcl_SetCommandInfo(Tcl_Interp *interp, const char *cmdName,
                         const Tcl_CmdInfo *infoPtr);
void Tcl_DeleteCommandFromToken(Tcl_Interp *interp, Tcl_Command command);
Tcl_Obj* Tcl_DuplicateObj(Tcl_Obj *objPtr);
Tcl_Obj* Tcl_ObjSetVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
                          Tcl_Obj *part2Ptr, Tcl_Obj *newValuePtr,
                          int flags);
void Tcl_AppendElement(Tcl_Interp *interp, const char *element);
int  Tcl_VarEval(Tcl_Interp *interp, ...);
void Tcl_AddErrorInfo(Tcl_Interp *interp, const char *message);
void Tcl_SetErrorCode(Tcl_Interp *interp, ...);
int  Tcl_PkgProvide(Tcl_Interp *interp, const char *name, const char *version);
Tcl_Obj* Tcl_NewLongObj(long longValue);
int  Tcl_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, long *longPtr);

// Trace support
typedef char* (Tcl_VarTraceProc)(ClientData clientData, Tcl_Interp *interp,
                                  const char *name1, const char *name2,
                                  int flags);
int  Tcl_TraceVar(Tcl_Interp *interp, const char *varName, int flags,
                   Tcl_VarTraceProc *proc, ClientData clientData);
void Tcl_UntraceVar(Tcl_Interp *interp, const char *varName, int flags,
                     Tcl_VarTraceProc *proc, ClientData clientData);

// Hash table functions (stubs for SWIG)
void Tcl_InitHashTable(Tcl_HashTable *tablePtr, int keyType);
void Tcl_DeleteHashTable(Tcl_HashTable *tablePtr);
Tcl_HashEntry* Tcl_CreateHashEntry(Tcl_HashTable *tablePtr,
                                    const char *key, int *newPtr);
Tcl_HashEntry* Tcl_FindHashEntry(Tcl_HashTable *tablePtr, const char *key);
void Tcl_DeleteHashEntry(Tcl_HashEntry *entryPtr);
void Tcl_SetHashValue(Tcl_HashEntry *entryPtr, ClientData value);
ClientData Tcl_GetHashValue(Tcl_HashEntry *entryPtr);
const char* Tcl_GetHashKey(Tcl_HashTable *tablePtr, Tcl_HashEntry *entryPtr);
Tcl_HashEntry* Tcl_FirstHashEntry(Tcl_HashTable *tablePtr,
                                   Tcl_HashSearch *searchPtr);
Tcl_HashEntry* Tcl_NextHashEntry(Tcl_HashSearch *searchPtr);

// ============================================================
// Memory
// ============================================================
void  Tcl_Free(char *ptr);
char* Tcl_Alloc(unsigned int size);

// ============================================================
// DString
// ============================================================
void  Tcl_DStringInit(Tcl_DString *dsPtr);
char* Tcl_DStringAppend(Tcl_DString *dsPtr, const char *bytes, int length);
void  Tcl_DStringFree(Tcl_DString *dsPtr);
char* Tcl_DStringValue(Tcl_DString *dsPtr);
int   Tcl_DStringLength(Tcl_DString *dsPtr);

// Glob-style string matching
int   Tcl_StringMatch(const char *str, const char *pattern);

#ifdef __cplusplus
}
#endif
