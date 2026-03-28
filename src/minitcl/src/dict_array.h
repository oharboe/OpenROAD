// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Dict, array, info, glob commands

#pragma once

#include "tcl.h"

namespace minitcl {

void registerDictCommands(Tcl_Interp *interp);
void registerArrayCommands(Tcl_Interp *interp);
void registerInfoCommand(Tcl_Interp *interp);
void registerGlobCommand(Tcl_Interp *interp);

}  // namespace minitcl
