"""Generate the runtime JAR allowlist from tracked validation metadata."""

load("@rules_decomp//toolchains/pypy:toolchain.bzl", "PYPY_TOOLCHAIN_TYPE")

def _supported_jars_header_impl(ctx):
    runtime = ctx.toolchains[PYPY_TOOLCHAIN_TYPE]
    output = ctx.actions.declare_file(ctx.label.name + ".hpp")
    args = ctx.actions.args()
    args.add("-B")
    args.add(ctx.file.generator.path)
    args.add("--manifest", ctx.file.manifest.path)
    args.add("--inventory", ctx.file.inventory.path)
    args.add("--output", output.path)
    ctx.actions.run(
        executable = runtime.interpreter,
        arguments = [args],
        inputs = depset(
            direct = [ctx.file.generator, ctx.file.manifest, ctx.file.inventory],
            transitive = [runtime.files],
        ),
        outputs = [output],
        mnemonic = "SupportedJarsHeader",
        progress_message = "Generating runtime JAR allowlist for %s" % ctx.label,
        env = {
            "PYTHONDONTWRITEBYTECODE": "1",
            "PYTHONHASHSEED": "0",
        },
    )
    return [DefaultInfo(files = depset([output]))]

supported_jars_header = rule(
    implementation = _supported_jars_header_impl,
    attrs = {
        "generator": attr.label(allow_single_file = [".py"], mandatory = True),
        "inventory": attr.label(allow_single_file = [".json"], mandatory = True),
        "manifest": attr.label(allow_single_file = True, mandatory = True),
    },
    toolchains = [PYPY_TOOLCHAIN_TYPE],
)
