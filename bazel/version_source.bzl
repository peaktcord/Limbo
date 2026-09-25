"""Generate the translation unit holding the build's commit id.

A genrule cannot read the workspace status files, so this is a rule:
ctx.info_file is the stable status that bazel/workspace_status.py writes into.
Depending on it is what ties this target to --stamp; a build without stamping
still gets a source file, just without the key.

Keep the library this feeds a leaf. `build --stamp` in .bazelrc means the
generated source changes whenever HEAD moves, so only that library and the
front ends linking it are invalidated by a commit.
"""

load("@rules_decomp//toolchains/pypy:toolchain.bzl", "PYPY_TOOLCHAIN_TYPE")

def _version_source_impl(ctx):
    runtime = ctx.toolchains[PYPY_TOOLCHAIN_TYPE]
    output = ctx.actions.declare_file(ctx.label.name + ".cpp")
    args = ctx.actions.args()
    args.add("-B")
    args.add(ctx.file.generator.path)
    args.add("--status", ctx.info_file.path)
    args.add("--output", output.path)
    ctx.actions.run(
        executable = runtime.interpreter,
        arguments = [args],
        inputs = depset(
            direct = [ctx.file.generator, ctx.info_file],
            transitive = [runtime.files],
        ),
        outputs = [output],
        mnemonic = "VersionSource",
        progress_message = "Generating build version for %s" % ctx.label,
        env = {
            "PYTHONDONTWRITEBYTECODE": "1",
            "PYTHONHASHSEED": "0",
        },
    )
    return [DefaultInfo(files = depset([output]))]

version_source = rule(
    implementation = _version_source_impl,
    attrs = {
        "generator": attr.label(allow_single_file = [".py"], mandatory = True),
    },
    toolchains = [PYPY_TOOLCHAIN_TYPE],
)
