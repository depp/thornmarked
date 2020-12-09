def _textures_impl(ctx):
    outputs = []
    base_args = ["-format=" + ctx.attr.format]
    if ctx.attr.mipmap:
        base_args.append("-mipmap")
    if ctx.attr.native:
        base_args.append("-native")
    if ctx.attr.dither != "":
        base_args.append("-dither=" + ctx.attr.dither)
    if ctx.attr.strips:
        base_args.append("-strips")
    if ctx.attr.anchor:
        base_args.append("-anchor=" + ctx.attr.anchor)
    suffix = ctx.attr.suffix + ".texture"
    for src in ctx.files.srcs:
        name = src.basename
        idx = name.find(".")
        if idx >= 0:
            name = name[:idx]
        out = ctx.actions.declare_file(name + suffix)
        outputs.append(out)
        ctx.actions.run(
            outputs = [out],
            inputs = [src],
            progress_message = "Converting texture %s" % src.short_path,
            executable = ctx.executable._converter,
            arguments = base_args + [
                "-output=" + out.path,
                "-input=" + src.path,
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
        "mipmap": attr.bool(
            default = False,
        ),
        "native": attr.bool(
            default = False,
        ),
        "dither": attr.string(),
        "suffix": attr.string(),
        "strips": attr.bool(),
        "anchor": attr.string(),
        "_converter": attr.label(
            default = Label("//tools/textureconvert"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
