def _asset_header_impl(ctx):
    manifest = ctx.file.manifest
    header = ctx.actions.declare_file(ctx.attr.header)
    ctx.actions.run(
        outputs = [header],
        inputs = [manifest],
        progress_message = "Creating asset header %s" % header.short_path,
        executable = ctx.executable._makepak,
        arguments = [
            "-manifest=" + manifest.path,
            "-out-header=" + header.path,
        ],
    )
    files = depset([header])
    return [
        DefaultInfo(files = files),
        CcInfo(
            compilation_context = cc_common.create_compilation_context(
                headers = files,
            ),
        ),
    ]

asset_header = rule(
    implementation = _asset_header_impl,
    attrs = {
        "manifest": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
        "header": attr.string(
            mandatory = True,
        ),
        "_makepak": attr.label(
            default = Label("//tools/makepak"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
    provides = [CcInfo],
)

def _join(path, *paths):
    result = path
    for p in paths:
        if not p or p == ".":
            pass
        elif path.startswith("/"):
            result = p
        elif not result or result.endswith("/"):
            result += p
        else:
            result += "/" + p
    if result == "":
        result = "."
    return result

def _asset_data_impl(ctx):
    manifest = ctx.file.manifest
    out = ctx.actions.declare_file(ctx.label.name + ".pak")
    arguments = [
        "-manifest=" + manifest.path,
        "-out-data=" + out.path,
    ]
    for dirname in ctx.attr.dirs:
        arguments += [
            "-dir=" + _join(ctx.bin_dir.path, ctx.label.workspace_root, ctx.label.package, dirname),
            "-dir=" + _join(ctx.label.workspace_root, ctx.label.package, dirname),
        ]
    ctx.actions.run(
        outputs = [out],
        inputs = [manifest] + ctx.files.srcs,
        progress_message = "Creating asset package %s" % out.short_path,
        executable = ctx.executable._makepak,
        arguments = arguments,
    )
    return [DefaultInfo(files = depset([out]))]

asset_data = rule (
    implementation = _asset_data_impl,
    attrs = {
        "manifest": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
        "srcs": attr.label_list(
            allow_files = True,
        ),
        "dirs": attr.string_list(
            default = ["."],
        ),
        "_makepak": attr.label(
            default = Label("//tools/makepak"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
