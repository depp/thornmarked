def _n64_rom_impl(ctx):
    program = ctx.file.program
    data = ctx.file.data
    bootcode = ctx.file.bootcode
    out = ctx.actions.declare_file(ctx.label.name + ".n64")
    ctx.actions.run(
        outputs = [out],
        inputs = [program, data, bootcode],
        progress_message = "Creating ROM %s" % out.short_path,
        executable = ctx.executable._makemask,
        arguments = [
            "-program=" + program.path,
            "-pak=" + data.path,
            "-bootcode=" + bootcode.path,
            "-output=" + out.path,
        ],
    )
    return [DefaultInfo(files = depset([out]))]

n64_rom = rule(
    implementation = _n64_rom_impl,
    attrs = {
        "program": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
        "data": attr.label(
            allow_single_file = True,
        ),
        "bootcode": attr.label(
            default = Label("//sdk:boot6102.bin"),
            allow_single_file = True,
        ),
        "_makemask": attr.label(
            default = Label("//tools/makemask"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
