// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - File and I/O commands

#include "file_cmds.h"
#include "interp.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

namespace minitcl {

// Open file channel management
static std::map<std::string, FILE *> &openChannels() {
    static std::map<std::string, FILE *> channels;
    return channels;
}

static int nextChannelId = 1;

// ============================================================
// file command
// ============================================================

static int fileCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }
    const char *sub = Tcl_GetString(objv[1]);

    if (strcmp(sub, "exists") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        bool exists = std::filesystem::exists(Tcl_GetString(objv[2]));
        Tcl_SetObjResult(interp, Tcl_NewIntObj(exists ? 1 : 0));
        return TCL_OK;
    }

    if (strcmp(sub, "dirname") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto p = std::filesystem::path(Tcl_GetString(objv[2])).parent_path();
        std::string result = p.empty() ? "." : p.string();
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "tail") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto result = std::filesystem::path(Tcl_GetString(objv[2])).filename().string();
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "extension") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto result = std::filesystem::path(Tcl_GetString(objv[2])).extension().string();
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "rootname") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto result = std::filesystem::path(Tcl_GetString(objv[2])).stem().string();
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "join") == 0) {
        std::filesystem::path result;
        for (int i = 2; i < objc; i++) {
            if (i == 2) result = Tcl_GetString(objv[i]);
            else result /= Tcl_GetString(objv[i]);
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.string().c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "normalize") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        try {
            auto result = std::filesystem::weakly_canonical(Tcl_GetString(objv[2])).string();
            Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        } catch (...) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_GetString(objv[2]), -1));
        }
        return TCL_OK;
    }

    if (strcmp(sub, "nativename") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_GetString(objv[2]), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "isfile") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        bool is = std::filesystem::is_regular_file(Tcl_GetString(objv[2]));
        Tcl_SetObjResult(interp, Tcl_NewIntObj(is ? 1 : 0));
        return TCL_OK;
    }

    if (strcmp(sub, "isdirectory") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        bool is = std::filesystem::is_directory(Tcl_GetString(objv[2]));
        Tcl_SetObjResult(interp, Tcl_NewIntObj(is ? 1 : 0));
        return TCL_OK;
    }

    if (strcmp(sub, "readable") == 0 || strcmp(sub, "writable") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        bool exists = std::filesystem::exists(Tcl_GetString(objv[2]));
        Tcl_SetObjResult(interp, Tcl_NewIntObj(exists ? 1 : 0));
        return TCL_OK;
    }

    if (strcmp(sub, "size") == 0) {
        if (objc != 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        try {
            auto sz = std::filesystem::file_size(Tcl_GetString(objv[2]));
            Tcl_SetObjResult(interp, Tcl_NewWideIntObj(sz));
        } catch (...) {
            getImpl(interp)->result = "could not get file size";
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    if (strcmp(sub, "mkdir") == 0) {
        for (int i = 2; i < objc; i++) {
            std::filesystem::create_directories(Tcl_GetString(objv[i]));
        }
        return TCL_OK;
    }

    if (strcmp(sub, "delete") == 0) {
        bool force = false;
        int startIdx = 2;
        if (startIdx < objc && strcmp(Tcl_GetString(objv[startIdx]), "-force") == 0) {
            force = true;
            startIdx++;
        }
        for (int i = startIdx; i < objc; i++) {
            if (force) std::filesystem::remove_all(Tcl_GetString(objv[i]));
            else std::filesystem::remove(Tcl_GetString(objv[i]));
        }
        return TCL_OK;
    }

    if (strcmp(sub, "copy") == 0) {
        bool force = false;
        int startIdx = 2;
        if (startIdx < objc && strcmp(Tcl_GetString(objv[startIdx]), "-force") == 0) {
            force = true;
            startIdx++;
        }
        if (startIdx + 1 >= objc) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        auto opts = force ? std::filesystem::copy_options::overwrite_existing
                          : std::filesystem::copy_options::none;
        std::filesystem::copy(Tcl_GetString(objv[startIdx]),
                               Tcl_GetString(objv[startIdx + 1]), opts);
        return TCL_OK;
    }

    if (strcmp(sub, "rename") == 0) {
        if (objc < 4) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        int srcIdx = 2;
        if (strcmp(Tcl_GetString(objv[2]), "-force") == 0) srcIdx = 3;
        if (srcIdx + 1 >= objc) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
        std::filesystem::rename(Tcl_GetString(objv[srcIdx]),
                                 Tcl_GetString(objv[srcIdx + 1]));
        return TCL_OK;
    }

    if (strcmp(sub, "channels") == 0) {
        std::string result = "stdin stdout stderr";
        for (auto &[name, fp] : openChannels()) {
            result += ' ';
            result += name;
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
        return TCL_OK;
    }

    getImpl(interp)->result = std::string("unknown file subcommand \"") + sub + "\"";
    return TCL_ERROR;
}

// ============================================================
// source command
// ============================================================

static int sourceCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args: should be \"source fileName\"";
        return TCL_ERROR;
    }
    // Skip -encoding flag if present
    int fileIdx = 1;
    if (objc > 2 && strcmp(Tcl_GetString(objv[1]), "-encoding") == 0) {
        fileIdx = 3;
    }
    if (fileIdx >= objc) {
        getImpl(interp)->result = "wrong # args";
        return TCL_ERROR;
    }
    return Tcl_EvalFile(interp, Tcl_GetString(objv[fileIdx]));
}

// ============================================================
// I/O commands: open, close, gets, read, eof, flush
// ============================================================

static int openCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    const char *filename = Tcl_GetString(objv[1]);
    const char *mode = (objc >= 3) ? Tcl_GetString(objv[2]) : "r";
    FILE *fp = fopen(filename, mode);
    if (!fp) {
        getImpl(interp)->result = std::string("couldn't open \"") + filename + "\"";
        return TCL_ERROR;
    }
    char chanName[32];
    snprintf(chanName, sizeof(chanName), "file%d", nextChannelId++);
    openChannels()[chanName] = fp;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(chanName, -1));
    return TCL_OK;
}

static int closeCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc != 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    const char *chanName = Tcl_GetString(objv[1]);
    auto &channels = openChannels();
    auto it = channels.find(chanName);
    if (it == channels.end()) {
        getImpl(interp)->result = std::string("can not find channel named \"") + chanName + "\"";
        return TCL_ERROR;
    }
    fclose(it->second);
    channels.erase(it);
    return TCL_OK;
}

static FILE *getChannel(Tcl_Interp *interp, const char *name) {
    if (strcmp(name, "stdin") == 0) return stdin;
    if (strcmp(name, "stdout") == 0) return stdout;
    if (strcmp(name, "stderr") == 0) return stderr;
    auto &channels = openChannels();
    auto it = channels.find(name);
    if (it != channels.end()) return it->second;
    getImpl(interp)->result = std::string("can not find channel named \"") + name + "\"";
    return nullptr;
}

static int getsCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2 || objc > 3) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    FILE *fp = getChannel(interp, Tcl_GetString(objv[1]));
    if (!fp) return TCL_ERROR;

    std::string line;
    int ch;
    while ((ch = fgetc(fp)) != EOF && ch != '\n') {
        line += static_cast<char>(ch);
    }

    if (objc == 3) {
        Tcl_SetVar(interp, Tcl_GetString(objv[2]), line.c_str(), 0);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(ch == EOF && line.empty() ? -1 : line.size()));
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(line.c_str(), line.size()));
    }
    return TCL_OK;
}

static int readCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    bool nonewline = false;
    int chanIdx = 1;
    if (strcmp(Tcl_GetString(objv[1]), "-nonewline") == 0) {
        nonewline = true;
        chanIdx = 2;
    }
    if (chanIdx >= objc) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    FILE *fp = getChannel(interp, Tcl_GetString(objv[chanIdx]));
    if (!fp) return TCL_ERROR;

    std::string content;
    int ch;
    while ((ch = fgetc(fp)) != EOF) content += static_cast<char>(ch);
    if (nonewline && !content.empty() && content.back() == '\n') content.pop_back();
    Tcl_SetObjResult(interp, Tcl_NewStringObj(content.c_str(), content.size()));
    return TCL_OK;
}

static int eofCmd(ClientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const objv[]) {
    if (objc != 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    FILE *fp = getChannel(interp, Tcl_GetString(objv[1]));
    if (!fp) return TCL_ERROR;
    Tcl_SetObjResult(interp, Tcl_NewIntObj(feof(fp) ? 1 : 0));
    return TCL_OK;
}

static int flushCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    if (objc != 2) { getImpl(interp)->result = "wrong # args"; return TCL_ERROR; }
    FILE *fp = getChannel(interp, Tcl_GetString(objv[1]));
    if (!fp) return TCL_ERROR;
    fflush(fp);
    return TCL_OK;
}

static int fconfigureCmd(ClientData, Tcl_Interp *, int, Tcl_Obj *const[]) {
    // No-op stub for fconfigure
    return TCL_OK;
}

void registerFileCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "file", fileCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "source", sourceCmd, nullptr, nullptr);
}

void registerIOCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "open", openCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "close", closeCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "gets", getsCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "read", readCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "eof", eofCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "flush", flushCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "fconfigure", fconfigureCmd, nullptr, nullptr);
}

}  // namespace minitcl
