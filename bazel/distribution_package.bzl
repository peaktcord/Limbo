"""Rules for assembling relocatable native and browser distributions."""

load("@rules_decomp//toolchains/pypy:toolchain.bzl", "PYPY_TOOLCHAIN_TYPE")


def _distribution_package_impl(ctx):
    runtime = ctx.toolchains[PYPY_TOOLCHAIN_TYPE]
    output = ctx.actions.declare_directory(ctx.label.name)

    root_files = list(ctx.files.files)
    if ctx.attr.binary:
        binary = ctx.executable.binary
        root_files.append(binary)

        runfiles = ctx.attr.binary[DefaultInfo].default_runfiles.files.to_list()
        root_files.extend([
            f
            for f in runfiles
            if f.extension.lower() in ctx.attr.runtime_extensions
        ])

    named_files = []
    for target, destination in ctx.attr.named_files.items():
        files = target[DefaultInfo].files.to_list()
        if len(files) != 1:
            fail("%s must provide exactly one file" % target.label)
        named_files.append((files[0], destination))

    inputs = root_files + ctx.files.docs + [pair[0] for pair in named_files]
    inputs.append(ctx.file._packager)

    args = ctx.actions.args()
    args.add("-B")
    args.add(ctx.file._packager.path)
    args.add(output.path)
    for source in root_files + ctx.files.docs:
        args.add("--root-file")
        args.add(source.path)
    for source, destination in named_files:
        args.add("--named-file")
        args.add("%s=%s" % (destination, source.path))

    ctx.actions.run(
        executable = runtime.interpreter,
        arguments = [args],
        inputs = depset(direct = inputs, transitive = [runtime.files]),
        outputs = [output],
        mnemonic = "PackageDistribution",
        progress_message = "Packaging %s" % ctx.label.name,
        env = {
            "PYTHONDONTWRITEBYTECODE": "1",
            "PYTHONHASHSEED": "0",
        },
    )
    return [DefaultInfo(files = depset([output]))]


distribution_package = rule(
    implementation = _distribution_package_impl,
    doc = "Assembles a relocatable distribution directory.",
    attrs = {
        "binary": attr.label(
            executable = True,
            cfg = "target",
            doc = "Optional executable; its runtime libraries are included automatically.",
        ),
        "docs": attr.label_list(
            allow_files = True,
            doc = "Documentation copied into the package root.",
        ),
        "files": attr.label_list(
            allow_files = True,
            doc = "Additional files copied into the package root.",
        ),
        "named_files": attr.label_keyed_string_dict(
            allow_files = True,
            doc = "Single-file labels mapped to relative package paths.",
        ),
        "runtime_extensions": attr.string_list(
            default = ["dll"],
            doc = "Runfile extensions copied beside the executable.",
        ),
        "_packager": attr.label(
            default = Label("//bazel:package_distribution.py"),
            allow_single_file = [".py"],
        ),
    },
    toolchains = [PYPY_TOOLCHAIN_TYPE],
)
