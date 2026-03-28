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
]]
