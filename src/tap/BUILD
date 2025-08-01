# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "tap",
    srcs = [
        "src/tapcell.cpp",
    ],
    hdrs = [
        "include/tap/tapcell.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb",
        "//src/utl",
        "@boost.polygon",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "include/tap/tapcell.h",
        "src/MakeTapcell.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/tap/MakeTapcell.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//:ord",
        "//src/odb",
        "//src/utl",
        "@boost.polygon",
        "@boost.stacktrace",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/tapcell.tcl",
    ],
    char_array_name = "tap_tcl_inits",
    namespace = "tap",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/tapcell.i",
        "//:error_swig",
    ],
    module = "tap",
    namespace_prefix = "tap",
    root_swig_src = "src/tapcell.i",
    swig_includes = [
        "src/tap/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
