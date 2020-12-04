def _dirname(path):
    dirname, sep, _ = path.rpartition("/")
    if not dirname:
        return sep
    return dirname.rstrip("/")

def _asset_header_impl(ctx):
    manifest = ctx.file.manifest
    headers = [
        "data.h",
        "model.h",
        "pak.h",
        "texture.h",
        "track.h",
    ]
    header_files = [
        ctx.actions.declare_file(ctx.attr.prefix + header)
        for header in headers
    ]
    ctx.actions.run(
        outputs = header_files,
        inputs = [manifest],
        progress_message = "Creating asset headers for %s" % manifest.short_path,
        executable = ctx.executable._makepak,
        arguments = [
            "-manifest=" + manifest.path,
            "-out-code-dir=" + _dirname(header_files[0].path),
            "-out-code-prefix=" + ctx.attr.prefix,
        ],
    )
    files = depset(header_files)
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
        "prefix": attr.string(),
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
    stats = ctx.actions.declare_file(ctx.label.name + ".txt")
    arguments = [
        "-manifest=" + manifest.path,
        "-out-data=" + out.path,
        "-out-data-stats=" + stats.path,
    ]
    for dirname in ctx.attr.dirs:
        arguments += [
            "-dir=" + _join(ctx.bin_dir.path, ctx.label.workspace_root, ctx.label.package, dirname),
            "-dir=" + _join(ctx.label.workspace_root, ctx.label.package, dirname),
        ]
    ctx.actions.run(
        outputs = [out, stats],
        inputs = [manifest] + ctx.files.srcs,
        progress_message = "Creating asset package %s" % out.short_path,
        executable = ctx.executable._makepak,
        arguments = arguments,
    )
    return [DefaultInfo(files = depset([out]))]

asset_data = rule(
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
