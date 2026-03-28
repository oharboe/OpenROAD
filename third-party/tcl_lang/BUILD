# Shim: provide @tcl_lang//:tcl using minitcl headers and library.
# This allows src/sta's BUILD file to remain unpatched while
# getting minitcl instead of real Tcl at build time.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "tcl",
    hdrs = glob(["include/*.h"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)
