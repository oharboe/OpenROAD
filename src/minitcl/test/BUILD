# SPDX-License-Identifier: BSD-3-Clause
# MiniTcl tests - idiomatic Bazel C++ tests, zero external dependencies

load("@rules_cc//cc:cc_test.bzl", "cc_test")

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
]]
