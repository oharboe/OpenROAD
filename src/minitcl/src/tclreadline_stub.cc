// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Mock tclreadline stubs

#include "tclreadline.h"

int Tclreadline_Init(Tcl_Interp *) {
    return TCL_OK;
}

int Tclreadline_SafeInit(Tcl_Interp *) {
    return TCL_OK;
}
