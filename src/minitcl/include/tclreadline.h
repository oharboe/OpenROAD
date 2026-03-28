// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Mock tclreadline.h for source compatibility
//
// Provides stub declarations so that code guarded by ENABLE_READLINE
// compiles without the real tclreadline library.

#pragma once

#include "tcl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TCLRL_LIBRARY "/dev/null"
#define TCLRL_VERSION_STR "2.3.8"

int Tclreadline_Init(Tcl_Interp *interp);
int Tclreadline_SafeInit(Tcl_Interp *interp);

#ifdef __cplusplus
}
#endif
