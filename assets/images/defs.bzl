def _textures_impl(ctx):
    outputs = []
    base_args = ["-format=" + ctx.attr.format]
    for src in ctx.files.srcs:
        name = src.basename
        idx = name.find(".")
        if idx >= 0:
            name = name[:idx]
        out = ctx.actions.declare_file(name + ".texture")
        outputs.append(out)
        ctx.actions.run(
            outputs = [out],
            inputs = [src],
            progress_message = "Converting texture %s" % src.short_path,
            executable = ctx.executable._converter,
            arguments = base_args + [
                "-output=" + out.path,
                src.path,
            ],
        )
    return [DefaultInfo(files = depset(outputs))]

textures = rule(
    implementation = _textures_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            mandatory = True,
        ),
        "format": attr.string(
            mandatory = True,
        ),
        "_converter": attr.label(
            default = Label("//tools/textureconvert"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
