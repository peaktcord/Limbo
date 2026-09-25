"""Declared refresh action for the private gameplay scenario trace."""


def _gameplay_trace_impl(ctx):
    scratch = ctx.actions.declare_directory(ctx.label.name + "_scratch")
    output = ctx.outputs.out
    runner = ctx.attr.runner[DefaultInfo].files_to_run
    ctx.actions.run(
        executable = runner,
        arguments = [
            ctx.attr.resource_dir,
            "--emit",
            output.path,
        ],
        inputs = ctx.files.resources,
        outputs = [output, scratch],
        mnemonic = "GameplayTrace",
        progress_message = "Generating %s for review" % output.short_path,
        env = {
            "TEST_TMPDIR": scratch.path,
        },
    )
    return [DefaultInfo(files = depset([output]))]


gameplay_trace = rule(
    implementation = _gameplay_trace_impl,
    doc = "Runs a private deterministic scenario and emits a candidate trace.",
    attrs = {
        "out": attr.output(mandatory = True),
        "resource_dir": attr.string(mandatory = True),
        "resources": attr.label_list(allow_files = True, mandatory = True),
        "runner": attr.label(executable = True, cfg = "exec", mandatory = True),
    },
)
