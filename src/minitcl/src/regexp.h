// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Regexp support (std::regex backend)

#pragma once

#include "tcl.h"

namespace minitcl {

void registerRegexpCommands(Tcl_Interp *interp);

// Initialize the C API regexp stubs (Tcl_GetRegExpFromObj, Tcl_RegExpExec)
void initRegexpApi();

}  // namespace minitcl
