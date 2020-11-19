_DEFAULT_TEMPLATE = Label("@thornmarked//base/bazel:tools.BUILD.tpl")

def _impl(repository_ctx):
    tools = repository_ctx.attr.tools
    build_file_template = repository_ctx.attr.build_file_template

    for tool in tools:
        path = repository_ctx.which(tool)
        repository_ctx.symlink(path, repository_ctx.path(tool))

    substitutions = {
        "%{tools}": repr(tools),
    }
    repository_ctx.template("BUILD.bazel", build_file_template, substitutions)

local_tools_repository = repository_rule(
    implementation = _impl,
    local = True,
    attrs = {
        "tools": attr.string_list(),
        "build_file_template": attr.label(
            default = _DEFAULT_TEMPLATE,
            allow_files = True,
        ),
    },
)
