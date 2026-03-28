# SPDX-License-Identifier: BSD-3-Clause
# MiniTcl - Minimal Tcl interpreter for OpenROAD

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "minitcl",
    srcs = glob(["src/*.cc"]),
    hdrs = glob(["src/*.h", "include/*.h"]),
    # No ENABLE_READLINE: minitcl provides its own interactive loop in Tcl_Main
    includes = ["include"],
)
