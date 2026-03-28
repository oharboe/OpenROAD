# SPDX-License-Identifier: BSD-3-Clause
# MiniTcl tests - idiomatic Bazel C++ tests, zero external dependencies

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

[cc_test(
    name = test_name,
    srcs = [test_name + ".cc"],
    deps = [
        "//src/minitcl",
        "@googletest//:gtest_main",
    ],
) for test_name in [
    "interp_test",
    "obj_test",
    "eval_test",
    "proc_test",
    "control_test",
    "expr_test",
    "string_test",
    "list_test",
    "regexp_test",
    "namespace_test",
    "dict_test",
    "file_test",
    "channel_test",
    "misc_test",
    "swig_compat_test",
    "integration_test",
]]

# Tests that need access to internal headers
[cc_test(
    name = test_name,
    srcs = [test_name + ".cc"],
    includes = ["../"],
    deps = [
        "//src/minitcl",
        "@googletest//:gtest_main",
    ],
) for test_name in [
    "parser_test",
    "vfs_test",
]]

# OpenROAD integration tests: run .tcl scripts through the openroad binary
# to verify minitcl works end-to-end with SWIG commands, STA init, etc.

_OPENROAD_TCL_TESTS = {
    "tcl_smoke": [],
    "tcl_sta_commands": [],
    "tcl_read_design": ["//test:regression_resources"],
    "tcl_floorplan": ["//test:regression_resources"],
    "tcl_placement": ["//test:regression_resources"],
    "tcl_route": ["//test:regression_resources"],
}

[sh_test(
    name = test_name,
    srcs = ["openroad_tcl_test.sh"],
    args = [
        "$(rootpath //:openroad)",
        "$(rootpath %s.tcl)" % test_name,
    ],
    data = [
        "//:openroad",
        "%s.tcl" % test_name,
    ] + extra_data,
    size = "medium",
) for test_name, extra_data in _OPENROAD_TCL_TESTS.items()]
