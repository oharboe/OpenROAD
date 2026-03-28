// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Evaluation engine

#pragma once

#include <string>
#include <vector>

#include "tcl.h"

namespace minitcl {

// Perform variable substitution ($var, ${var}, $::ns::var, $arr(key))
// and command substitution ([cmd args...]) on a string.
std::string substitute(Tcl_Interp *interp, const std::string &str, int *code);

// Evaluate a single parsed command (after substitution).
int evalCommand(Tcl_Interp *interp, const std::vector<std::string> &words);

// Register built-in commands (set, unset, puts, etc.)
void registerBuiltins(Tcl_Interp *interp);

}  // namespace minitcl
