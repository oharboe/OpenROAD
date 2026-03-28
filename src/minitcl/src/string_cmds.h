// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - String and list commands

#pragma once

#include <string>
#include <vector>

#include "tcl.h"

namespace minitcl {

void registerStringCommands(Tcl_Interp *interp);
void registerListCommands(Tcl_Interp *interp);

// Parse a Tcl list string into elements (used by multiple commands)
std::vector<std::string> parseList(const char *listStr);

}  // namespace minitcl
