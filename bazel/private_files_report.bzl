"""Generate a local-only report from a verified set of private files."""

load("@rules_decomp//decomp:verified_inputs.bzl", "VerifiedFilesInfo")
load("@rules_decomp//toolchains/pypy:toolchain.bzl", "PYPY_TOOLCHAIN_TYPE")

def _short_path(file):
    return file.short_path

def _private_files_report_impl(ctx):
    runtime = ctx.toolchains[PYPY_TOOLCHAIN_TYPE]
    verified = ctx.attr.input[VerifiedFilesInfo]
    private_files = sorted(
        verified.files.to_list(),
        key = _short_path,
    )
    output = ctx.actions.declare_file(ctx.label.name + ".json")

    args = ctx.actions.args()
    args.add("-B")
    args.add(ctx.file.reporter.path)
    args.add("--output", output.path)
    for private_file in private_files:
        args.add("--input", private_file.path)

    ctx.actions.run(
        executable = runtime.interpreter,
        arguments = [args],
        inputs = depset(
            direct = [
                ctx.file.reporter,
                verified.verification,
            ] + private_files,
            transitive = [runtime.files],
        ),
        outputs = [output],
        mnemonic = "PrivateFilesReport",
        progress_message = "Generating private-files report for %s" % ctx.label,
        execution_requirements = {
            "no-remote": "1",
            "no-remote-cache": "1",
            "no-remote-exec": "1",
        },
        env = {
            "PYTHONDONTWRITEBYTECODE": "1",
            "PYTHONHASHSEED": "0",
        },
    )
    return [DefaultInfo(files = depset([output]))]

private_files_report = rule(
    implementation = _private_files_report_impl,
    attrs = {
        "input": attr.label(
            mandatory = True,
            providers = [VerifiedFilesInfo],
        ),
        "reporter": attr.label(
            allow_single_file = [".py"],
            mandatory = True,
        ),
    },
    toolchains = [PYPY_TOOLCHAIN_TYPE],
)

def _verified_files_data_impl(ctx):
    verified = ctx.attr.input[VerifiedFilesInfo]
    files = verified.files.to_list() + [verified.verification]
    return [
        DefaultInfo(
            files = depset(files),
            runfiles = ctx.runfiles(files = files),
        ),
    ]

verified_files_data = rule(
    implementation = _verified_files_data_impl,
    attrs = {
        "input": attr.label(
            mandatory = True,
            providers = [VerifiedFilesInfo],
        ),
    },
)
