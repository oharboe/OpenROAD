// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Namespace, upvar, uplevel commands

#pragma once

#include "tcl.h"

namespace minitcl {

void registerNamespaceCommands(Tcl_Interp *interp);

}  // namespace minitcl
