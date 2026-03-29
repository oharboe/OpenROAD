# SPDX-License-Identifier: BSD-3-Clause
# MiniTcl - Minimal Tcl interpreter for OpenROAD

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Build flag: --//src/minitcl:use_minitcl (default: true)
# Set to false to use system Tcl instead of minitcl.
bool_flag(
    name = "use_minitcl",
    build_setting_default = True,
)

config_setting(
    name = "minitcl_enabled",
    flag_values = {":use_minitcl": "True"},
)

# The cc_library that all BUILD files depend on.
# When use_minitcl is true (default), provides the minitcl implementation.
# When false, re-exports @tcl_lang//:tcl (system Tcl from BCR).
#
# To use system Tcl: remove the local_path_override for tcl_lang in
# MODULE.bazel, then build with --//src/minitcl:use_minitcl=false
alias(
    name = "minitcl",
    actual = select({
        ":minitcl_enabled": ":minitcl_impl",
        "//conditions:default": "@tcl_lang//:tcl",
    }),
)

cc_library(
    name = "minitcl_impl",
    srcs = glob(["src/*.cc"]),
    hdrs = glob(["src/*.h", "include/*.h"]),
    # No ENABLE_READLINE: minitcl provides its own interactive loop in Tcl_Main
    includes = ["include"],
)
