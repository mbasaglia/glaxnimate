#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2021 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: GPL-3.0-or-later

import os
import shlex
import pathlib
import argparse
import subprocess


class Variable:
    def __init__(self, cmake_name, *a, **kw):
        self.cmake_name = cmake_name
        self.value = kw.pop("fixed_value", None)
        self.argparse_args = a
        self.argparse_kwargs = kw
        self.default_callable = self.argparse_kwargs.pop("default_callable", None)

        self.dest = self.argparse_kwargs.get("dest", None)

        if not self.dest:
            if cmake_name:
                self.dest = cmake_name
            else:
                self.dest = self.argparse_args[0].strip("-").replace("-", "_")

        self.argparse_kwargs["dest"] = self.dest

    def add_to_parser(self, parser: argparse.ArgumentParser):
        if self.argparse_args or self.argparse_args:
            parser.add_argument(
                *self.argparse_args,
                **self.argparse_kwargs
            )

    def from_parser(self, variables, ns):
        if self.argparse_args or self.argparse_args:
            self.value = getattr(ns, self.dest)

        if self.value is None and self.default_callable:
            self.value = self.default_callable(variables, ns)

    def add_to_cmd(self, cmd):
        if self.cmake_name:
            cmd.append("-D%s=%s" % (self.cmake_name, str(self.value)))


class Variables:
    def __init__(self, parser: argparse.ArgumentParser()):
        self.variables = []
        self.vars = {}
        self.parser = parser
        self.args = []
        parser.add_argument("cmake_args", nargs="+", help="CMake arguments")

    def add(self, var: Variable) -> "Variables":
        self.variables.append(var)
        self.vars[var.dest] = var
        var.add_to_parser(self.parser)
        return self

    def var(self, *a, **kw) -> "Variables":
        return self.add(Variable(*a, **kw))

    def option(self, *a, **kw) -> "Variables":
        return self.add(Variable(None, *a, **kw))

    def fixed(self, cmake_name, value) -> "Variables":
        return self.add(Variable(cmake_name, fixed_value=value))

    def parse_args(self):
        ns = self.parser.parse_args()
        for var in self.variables:
            var.from_parser(self, ns)

        self.args = ns.cmake_args
        return ns

    def invoke(self):
        cmd = ["cmake"]
        for var in self.variables:
            var.add_to_cmd(cmd)

        print(" ".join(map(shlex.quote, cmd)))
        subprocess.call(cmd + self.args)

    def __getitem__(self, key):
        return self.vars[key].value

    def error(self, msg):
        self.parser.error(msg)


def build_qt_path(variables: Variables, ns):
    if ns.qt is None:
        variables.error("You must specify either --qt or --qt-path")

    path = ns.qt_home / ".".join(map(str, ns.qt))

    if ns.qt < (5, 13):
        abi = ns.ANDROID_ABI
        if abi == "x86":
            abi_qt = abi
        elif abi == "armeabi-v7a":
            abi_qt = "armv7"
        elif abi == "arm64-v8a":
            abi_qt = "armv64_v8a"
        path /= "android_" + abi_qt
    else:
        path /= "android"

    return path


def ndk_default(variables: Variables, ns):
    path = variables["ANDROID_SDK"] / "ndk"
    attempts = list(path.iterdir())
    if len(attempts) == 1:
        path /= attempts[0]
    else:
        variables.error("Could not determine --ndk automatically")
    return path


parser = argparse.ArgumentParser()
variables = Variables(parser)
variables.option(
    "--qt",
    type=lambda x: tuple(map(int, x.split("."))),
    help="Qt Version",
    default=None,
)
variables.option(
    "--qt-home",
    default=pathlib.Path.home() / "Qt",
    type=pathlib.Path,
    help="Home path to Qt if installed with the Qt installer",
)
variables.var(
    "CMAKE_FIND_ROOT_PATH",
    "--qt-path",
    default=None,
    type=pathlib.Path,
    help="Base path for Qt for android, by default based on --qt-home and --qt-version",
    default_callable=build_qt_path,
)
variables.var(
    "CMAKE_PREFIX_PATH",
    default_callable=lambda variables, ns: variables.vars["CMAKE_FIND_ROOT_PATH"].value / "lib/cmake"
)
variables.fixed(
    "ANDROID_STL", "c++_shared"
)
variables.var(
    "ANDROID_ABI",
    "--abi",
    choices=["armeabi-v7a", "arm64-v8a", "x86", "x86_64"],
    required=True
)
variables.var(
    "ANDROID_SDK",
    "--sdk",
    default=pathlib.Path.home() / "Android" / "Sdk",
    type=pathlib.Path,
)
variables.var(
    "ANDROID_NDK",
    "--ndk",
    type=pathlib.Path,
    default_callable=ndk_default
)
variables.var(
    "CMAKE_TOOLCHAIN_FILE",
    "--toolchain",
    default_callable=lambda variables, ns: variables.vars["ANDROID_NDK"].value / "build/cmake/android.toolchain.cmake"
)
variables.var(
    "ANDROID_PLATFORM",
    "--platform",
    type=int,
    default=29,
)
variables.var(
    "JAVA_HOME",
    "--java-home",
    default="/usr/lib/jvm/default-java",
)

variables.parse_args()

variables.invoke()
