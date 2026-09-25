"""Hermetic Node runner for generated JavaScript programs."""

load("@rules_nodejs//nodejs:toolchain.bzl", "NodeInfo")

def _runfile_path(ctx, file):
    if file.short_path.startswith("../"):
        return file.short_path[3:]
    return ctx.workspace_name + "/" + file.short_path

def _node_test_impl(ctx):
    programs = [file for file in ctx.attr.program[DefaultInfo].files.to_list() if file.path.endswith(".js")]
    if len(programs) != 1:
        fail("program must provide exactly one .js file")

    node_info = ctx.attr._node[NodeInfo]
    if node_info.node == None:
        fail("the Node toolchain must provide a hermetic executable")
    node = node_info.node
    program = programs[0]
    windows = ctx.target_platform_has_constraint(ctx.attr._windows[platform_common.ConstraintValueInfo])
    extension = ".bat" if windows else ".sh"
    launcher = ctx.actions.declare_file(ctx.label.name + extension)
    node_path = _runfile_path(ctx, node)
    program_path = _runfile_path(ctx, program)

    if windows:
        content = """@echo off
set "TEST_RUNFILES=%RUNFILES_DIR%"
if not defined TEST_RUNFILES set "TEST_RUNFILES=%~dp0{launcher}.runfiles"
"%TEST_RUNFILES%\\{node}" "%TEST_RUNFILES%\\{program}"
exit /b %ERRORLEVEL%
""".format(
            launcher = launcher.basename,
            node = node_path.replace("/", "\\"),
            program = program_path.replace("/", "\\"),
        )
    else:
        content = """#!/usr/bin/env bash
set -euo pipefail
TEST_RUNFILES="${{RUNFILES_DIR:-$0.runfiles}}"
exec "$TEST_RUNFILES/{node}" "$TEST_RUNFILES/{program}"
""".format(node = node_path, program = program_path)

    ctx.actions.write(launcher, content, is_executable = True)
    runfiles = ctx.runfiles(files = [node, program])
    runfiles = runfiles.merge(ctx.attr.program[DefaultInfo].default_runfiles)
    return [DefaultInfo(executable = launcher, runfiles = runfiles)]

node_test = rule(
    implementation = _node_test_impl,
    test = True,
    attrs = {
        "program": attr.label(mandatory = True),
        "_node": attr.label(default = "@rules_nodejs//nodejs:current_node_runtime"),
        "_windows": attr.label(default = "@platforms//os:windows"),
    },
)
