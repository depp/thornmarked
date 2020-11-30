def _models_impl(ctx):
    scale = ctx.attr.scale
    outputs = []
    base_args = ["-meter=64"]
    if ctx.attr.normals:
        base_args.append("-use-normals")
    if ctx.attr.primitive_color:
        base_args.append("-use-primitive-color")
    if ctx.attr.texcoords:
        base_args.append("-use-texcoords")
    if ctx.attr.axes:
        base_args.append("-axes=" + ctx.attr.axes)
    for src in ctx.files.srcs:
        name = src.basename
        idx = name.find(".")
        if idx >= 0:
            name = name[:idx]
        out = ctx.actions.declare_file(name + ".model")
        out_stats = ctx.actions.declare_file(name + ".txt")
        outputs.append(out)
        ctx.actions.run(
            outputs = [out, out_stats],
            inputs = [src],
            progress_message = "Converting model %s" % src.short_path,
            executable = ctx.executable._converter,
            arguments = base_args + [
                "-model=" + src.path,
                "-output=" + out.path,
                "-output-stats=" + out_stats.path,
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
            default = "meter",
        ),
        "normals": attr.bool(
            default = False,
        ),
        "primitive_color": attr.bool(
            default = False,
        ),
        "texcoords": attr.bool(
            default = False,
        ),
        "axes": attr.string(),
        "_converter": attr.label(
            default = Label("//tools/modelconvert"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
