// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Namespace, upvar, uplevel commands

#include "namespace.h"
#include "eval.h"
#include "interp.h"

#include <cstdlib>
#include <cstring>
#include <set>

namespace minitcl {

// Current namespace tracking (simple string-based)
static std::string &currentNamespace(InterpImpl *impl) {
    static std::string ns = "::";
    (void)impl;
    return ns;
}

// Resolve a command name relative to the current namespace
static std::string qualifyName(const std::string &name, const std::string &ns) {
    if (name.size() >= 2 && name[0] == ':' && name[1] == ':') {
        return name;  // Already fully qualified
    }
    if (ns == "::" || ns.empty()) {
        return "::" + name;
    }
    return ns + "::" + name;
}

static int namespaceCmd(ClientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args: should be \"namespace subcommand ?arg ...?\"";
        return TCL_ERROR;
    }

    auto *impl = getImpl(interp);
    const char *sub = Tcl_GetString(objv[1]);

    if (strcmp(sub, "eval") == 0) {
        if (objc < 3) {
            impl->result = "wrong # args: should be \"namespace eval name ?arg ...?\"";
            return TCL_ERROR;
        }
        if (objc == 3) return TCL_OK;  // no body = no-op
        std::string ns = Tcl_GetString(objv[2]);
        // Qualify the namespace
        if (ns.size() < 2 || ns[0] != ':' || ns[1] != ':') {
            auto &cur = currentNamespace(impl);
            if (cur == "::") ns = "::" + ns;
            else ns = cur + "::" + ns;
        }

        // Save and set current namespace
        std::string oldNs = currentNamespace(impl);
        currentNamespace(impl) = ns;

        // Concatenate remaining args
        std::string script;
        for (int i = 3; i < objc; i++) {
            if (i > 3) script += ' ';
            script += Tcl_GetString(objv[i]);
        }

        int code = Tcl_Eval(interp, script.c_str());
        currentNamespace(impl) = oldNs;
        return code;
    }

    if (strcmp(sub, "current") == 0) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj(currentNamespace(impl).c_str(), -1));
        return TCL_OK;
    }

    if (strcmp(sub, "export") == 0) {
        // Store exported command patterns (simplified: just track them)
        // In real Tcl this controls what "namespace import" can see.
        // For minitcl, we allow importing anything.
        return TCL_OK;
    }

    if (strcmp(sub, "import") == 0) {
        // Import commands from other namespaces
        bool force = false;
        int startIdx = 2;
        if (startIdx < objc && strcmp(Tcl_GetString(objv[startIdx]), "-force") == 0) {
            force = true;
            startIdx++;
        }
        (void)force;
        for (int i = startIdx; i < objc; i++) {
            std::string pattern = Tcl_GetString(objv[i]);
            // Pattern like ::ns::* or ::ns::cmd
            // Find all matching commands and create aliases
            size_t starPos = pattern.find('*');
            if (starPos != std::string::npos) {
                std::string prefix = pattern.substr(0, starPos);
                for (auto &[name, cmd] : impl->commands) {
                    std::string qualName = "::" + name;
                    if (qualName.find(prefix) == 0) {
                        // Extract short name (after last ::)
                        size_t lastColon = name.rfind("::");
                        std::string shortName = (lastColon != std::string::npos)
                            ? name.substr(lastColon + 2)
                            : name;
                        if (shortName != name) {
                            impl->commands[shortName] = cmd;
                        }
                    }
                }
            } else {
                // Single command import
                size_t lastColon = pattern.rfind("::");
                if (lastColon != std::string::npos) {
                    std::string shortName = pattern.substr(lastColon + 2);
                    std::string qualName = pattern;
                    if (qualName.substr(0, 2) == "::") qualName = qualName.substr(2);
                    auto it = impl->commands.find(qualName);
                    if (it != impl->commands.end()) {
                        impl->commands[shortName] = it->second;
                    }
                }
            }
        }
        return TCL_OK;
    }

    if (strcmp(sub, "children") == 0) {
        // Return list of child namespaces (simplified: scan command names)
        std::string parent = (objc >= 3) ? Tcl_GetString(objv[2]) : currentNamespace(impl);
        if (parent == "::") parent = "";
        std::set<std::string> children;
        for (auto &[name, cmd] : impl->commands) {
            // Check if command name starts with parent::
            if (!parent.empty() && name.find(parent.substr(2) + "::") == 0) {
                std::string rest = name.substr(parent.size());
                if (rest.size() > 2 && rest[0] == ':' && rest[1] == ':') {
                    rest = rest.substr(2);
                }
                size_t next = rest.find("::");
                if (next != std::string::npos) {
                    children.insert(parent + "::" + rest.substr(0, next));
                }
            }
        }
        std::string result;
        for (auto &c : children) {
            if (!result.empty()) result += ' ';
            result += c;
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), result.size()));
        return TCL_OK;
    }

    if (strcmp(sub, "unknown") == 0) {
        // Set the unknown command handler
        // For now, just store it as a variable
        if (objc >= 3) {
            Tcl_SetVar(interp, "::namespace_unknown", Tcl_GetString(objv[2]),
                        TCL_GLOBAL_ONLY);
        }
        return TCL_OK;
    }

    if (strcmp(sub, "upvar") == 0) {
        // namespace upvar ns var local ?var local ...?
        if (objc < 5 || (objc - 3) % 2 != 0) {
            impl->result = "wrong # args";
            return TCL_ERROR;
        }
        // Simplified: treat as global upvar
        if (impl->callStack.empty()) return TCL_OK;
        auto &frame = impl->callStack.back();
        for (int i = 3; i + 1 < objc; i += 2) {
            const char *remoteVar = Tcl_GetString(objv[i]);
            const char *localVar = Tcl_GetString(objv[i + 1]);
            frame.upvarLinks[localVar] = {-1, remoteVar};
        }
        return TCL_OK;
    }

    impl->result = std::string("unknown namespace subcommand \"") + sub + "\"";
    return TCL_ERROR;
}

static int upvarCmd(ClientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const objv[]) {
    auto *impl = getImpl(interp);
    if (impl->callStack.empty()) return TCL_OK;

    int idx = 1;
    int level = 1;  // default: caller's frame

    // Parse optional level
    if (objc > 1) {
        const char *first = Tcl_GetString(objv[1]);
        if (first[0] == '#') {
            level = -1;  // #0 = global
            if (first[1] != '0') level = atoi(first + 1);
            idx = 2;
        } else {
            char *end;
            long l = strtol(first, &end, 10);
            if (end != first && *end == '\0' && (objc - 2) % 2 == 0) {
                level = l;
                idx = 2;
            }
        }
    }

    if ((objc - idx) % 2 != 0) {
        impl->result = "wrong # args: should be \"upvar ?level? otherVar localVar ...\"";
        return TCL_ERROR;
    }

    auto &frame = impl->callStack.back();
    int targetFrame;
    if (level == -1 || (Tcl_GetString(objv[1])[0] == '#' && Tcl_GetString(objv[1])[1] == '0')) {
        targetFrame = -1;  // global
    } else {
        targetFrame = static_cast<int>(impl->callStack.size()) - 1 - level;
        if (targetFrame < 0) targetFrame = -1;  // global
    }

    for (int i = idx; i + 1 < objc; i += 2) {
        const char *otherVar = Tcl_GetString(objv[i]);
        const char *localVar = Tcl_GetString(objv[i + 1]);
        frame.upvarLinks[localVar] = {targetFrame, otherVar};
    }
    return TCL_OK;
}

static int uplevelCmd(ClientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const objv[]) {
    if (objc < 2) {
        getImpl(interp)->result = "wrong # args: should be \"uplevel ?level? script\"";
        return TCL_ERROR;
    }

    auto *impl = getImpl(interp);
    int idx = 1;
    int level = 1;

    // Parse optional level
    if (objc > 2) {
        const char *first = Tcl_GetString(objv[1]);
        if (first[0] == '#') {
            level = -1;  // absolute frame
            idx = 2;
        } else {
            char *end;
            long l = strtol(first, &end, 10);
            if (end != first && *end == '\0') {
                level = l;
                idx = 2;
            }
        }
    }

    // Concatenate remaining args as script
    std::string script;
    for (int i = idx; i < objc; i++) {
        if (i > idx) script += ' ';
        script += Tcl_GetString(objv[i]);
    }

    // Pop frames to reach target level
    int framesToPop = level;
    if (level == -1) framesToPop = impl->callStack.size();
    if (framesToPop > (int)impl->callStack.size()) framesToPop = impl->callStack.size();

    std::vector<CallFrame> saved;
    for (int i = 0; i < framesToPop; i++) {
        saved.push_back(std::move(impl->callStack.back()));
        impl->callStack.pop_back();
    }

    int code = Tcl_Eval(interp, script.c_str());

    // Restore frames
    for (int i = framesToPop - 1; i >= 0; i--) {
        impl->callStack.push_back(std::move(saved[i]));
    }

    return code;
}

static int variableCmd(ClientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *const objv[]) {
    // In a namespace, "variable" declares namespace-scoped variables.
    // In minitcl, we treat them as globals.
    auto *impl = getImpl(interp);

    for (int i = 1; i < objc; i++) {
        const char *varName = Tcl_GetString(objv[i]);
        // If next arg exists and is a value, set it
        if (i + 1 < objc) {
            const char *next = Tcl_GetString(objv[i + 1]);
            // Heuristic: if not another variable declaration, treat as value
            Tcl_SetVar(interp, varName, next, TCL_GLOBAL_ONLY);
            i++;
        }
        // If in a proc, create upvar link to global
        if (!impl->callStack.empty()) {
            impl->callStack.back().upvarLinks[varName] = {-1, varName};
        }
    }
    return TCL_OK;
}

static int renameCmd(ClientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const objv[]) {
    if (objc != 3) {
        getImpl(interp)->result = "wrong # args: should be \"rename oldName newName\"";
        return TCL_ERROR;
    }
    auto *impl = getImpl(interp);
    const char *oldName = Tcl_GetString(objv[1]);
    const char *newName = Tcl_GetString(objv[2]);

    auto it = impl->commands.find(oldName);
    if (it == impl->commands.end()) {
        impl->result = std::string("can't rename \"") + oldName + "\": command doesn't exist";
        return TCL_ERROR;
    }

    if (newName[0] == '\0') {
        // Delete the command
        if (it->second.deleteProc) it->second.deleteProc(it->second.clientData);
        impl->commands.erase(it);
    } else {
        impl->commands[newName] = it->second;
        it->second.deleteProc = nullptr;  // Don't call delete on old entry
        impl->commands.erase(it);
    }
    return TCL_OK;
}

void registerNamespaceCommands(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "namespace", namespaceCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "upvar", upvarCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "uplevel", uplevelCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "variable", variableCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "rename", renameCmd, nullptr, nullptr);
}

}  // namespace minitcl
