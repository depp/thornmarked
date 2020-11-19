def _models_impl(ctx):
    scale = ctx.attr.scale
    outputs = []
    for src in ctx.files.srcs:
        name = src.basename
        idx = name.find(".")
        if idx >= 0:
            name = name[:idx]
        out = ctx.actions.declare_file(name + ".dat")
        outputs.append(out)
        ctx.actions.run(
            outputs = [out],
            inputs = [src],
            progress_message = "Converting model %s" % src.short_path,
            executable = ctx.executable._converter,
            arguments = [
                "-model=" + src.path,
                "-output=" + out.path,
                "-scale=" + scale,
            ],
        )
    return [DefaultInfo(files = depset(outputs))]

models = rule(
    implementation = _models_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            mandatory = True,
        ),
        "scale": attr.string(
            mandatory = True,
        ),
        "_converter": attr.label(
            default = Label("//tools/modelconvert"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
