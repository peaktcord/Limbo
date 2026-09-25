"""Small declared-output rule for an Emscripten page shell."""

def _web_page_impl(ctx):
    ctx.actions.expand_template(
        template = ctx.file.template,
        output = ctx.outputs.out,
        substitutions = {
            "{{{ SCRIPT }}}": '<script src="{}"></script>'.format(ctx.attr.script_name),
        },
    )
    return DefaultInfo(files = depset([ctx.outputs.out]))

web_page = rule(
    implementation = _web_page_impl,
    attrs = {
        "out": attr.output(mandatory = True),
        "script_name": attr.string(mandatory = True),
        "template": attr.label(allow_single_file = True, mandatory = True),
    },
)
