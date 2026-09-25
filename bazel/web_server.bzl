"""Runnable local server for a browser build's declared files."""

load("@rules_decomp//toolchains/pypy:toolchain.bzl", "PYPY_TOOLCHAIN_TYPE")

def _runfile_path(ctx, file):
    if file.short_path.startswith("../"):
        return file.short_path[3:]
    return ctx.workspace_name + "/" + file.short_path

def _web_server_impl(ctx):
    pages = [
        file
        for file in ctx.attr.page[DefaultInfo].files.to_list()
        if file.basename == ctx.attr.page_name
    ]
    if len(pages) != 1:
        fail("page must provide exactly one %s" % ctx.attr.page_name)

    runtime = ctx.toolchains[PYPY_TOOLCHAIN_TYPE]
    page = pages[0]
    served = ctx.files.page + ctx.files.srcs

    for file in served:
        if file.dirname != page.dirname:
            fail("%s is not beside %s" % (file.short_path, page.short_path))

    windows = ctx.target_platform_has_constraint(
        ctx.attr._windows[platform_common.ConstraintValueInfo],
    )
    launcher = ctx.actions.declare_file(
        ctx.label.name + (".bat" if windows else ".sh"),
    )
    interpreter_path = _runfile_path(ctx, runtime.interpreter)
    server_path = _runfile_path(ctx, ctx.file._server)

    directory_path = _runfile_path(ctx, page).rsplit("/", 1)[0]

    if windows:
        separator = "\\"

        content = """@echo off
setlocal
set "SERVER_RUNFILES=%RUNFILES_DIR%"
if not defined SERVER_RUNFILES set "SERVER_RUNFILES=%~dp0{launcher}.runfiles"
set "PYTHONDONTWRITEBYTECODE=1"
"%SERVER_RUNFILES%{interpreter}" -B "%SERVER_RUNFILES%{server}" --directory "%SERVER_RUNFILES%{directory}" --port {port} %*
exit /b %ERRORLEVEL%
""".format(
            launcher = launcher.basename,
            interpreter = separator + interpreter_path.replace("/", separator),
            server = separator + server_path.replace("/", separator),
            directory = separator + directory_path.replace("/", separator),
            port = ctx.attr.port,
        )
    else:
        content = """#!/usr/bin/env bash
set -euo pipefail
SERVER_RUNFILES="${{RUNFILES_DIR:-$0.runfiles}}"
export PYTHONDONTWRITEBYTECODE=1
exec "$SERVER_RUNFILES/{interpreter}" -B "$SERVER_RUNFILES/{server}" \
    --directory "$SERVER_RUNFILES/{directory}" --port {port} "$@"
""".format(
            interpreter = interpreter_path,
            server = server_path,
            directory = directory_path,
            port = ctx.attr.port,
        )

    ctx.actions.write(launcher, content, is_executable = True)

    runfiles = ctx.runfiles(
        files = served + [runtime.interpreter, ctx.file._server],
        transitive_files = runtime.files,
    )
    runfiles = runfiles.merge(ctx.attr.page[DefaultInfo].default_runfiles)
    for target in ctx.attr.srcs:
        runfiles = runfiles.merge(target[DefaultInfo].default_runfiles)
    return [DefaultInfo(executable = launcher, runfiles = runfiles)]

web_server = rule(
    implementation = _web_server_impl,
    doc = "Serves a browser build on 127.0.0.1 from its runfiles.",
    executable = True,
    attrs = {
        "page": attr.label(
            mandatory = True,
            allow_files = True,
            doc = "Target providing the HTML entry page.",
        ),
        "page_name": attr.string(
            default = "index.html",
            doc = "Basename of the entry page within page.",
        ),
        "port": attr.int(
            default = 8000,
            doc = "Default port; overridden by --port after --.",
        ),
        "srcs": attr.label_list(
            allow_files = True,
            doc = "Additional files served beside the page.",
        ),
        "_server": attr.label(
            default = Label("//:tools/serve_web.py"),
            allow_single_file = [".py"],
        ),
        "_windows": attr.label(default = "@platforms//os:windows"),
    },
    toolchains = [PYPY_TOOLCHAIN_TYPE],
)
