// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - File and I/O commands

#pragma once

#include "tcl.h"

namespace minitcl {

void registerFileCommands(Tcl_Interp *interp);
void registerIOCommands(Tcl_Interp *interp);

}  // namespace minitcl
