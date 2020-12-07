def _audio_tracks_impl(ctx):
    outputs = []
    for src in ctx.files.srcs:
        name = src.basename
        idx = name.rfind(".")
        if idx >= 0:
            name = name[:idx]
        if src.extension == "flac":
            out_wav = ctx.actions.declare_file(name + ".wav")
            ctx.actions.run(
                outputs = [out_wav],
                inputs = [src],
                progress_message = "Decompressing %s" % src.short_path,
                executable = ctx.executable._flac,
                arguments = [
                    "--silent",
                    "--decode",
                    src.path,
                    "--output-name",
                    out_wav.path,
                ],
            )
            src = out_wav
        if src.extension in ["wav", "aif", "aiff"]:
            out_aifc = ctx.actions.declare_file(name + ".pcm.aifc")
            ctx.actions.run(
                outputs = [out_aifc],
                inputs = [src],
                progress_message = "Converting %s" % src.short_path,
                executable = ctx.executable._sox,
                arguments = [
                    src.path,
                    "--bits",
                    "16",
                    "--rate",
                    ctx.attr.rate,
                    "--channels",
                    "1",
                    out_aifc.path,
                ],
            )
            src = out_aifc
        if src.extension != "aifc":
            fail("unknown audio track format: %s" % src.short_path)
        out_table = ctx.actions.declare_file(name + ".table.txt")
        ctx.actions.run_shell(
            outputs = [out_table],
            inputs = [src],
            progress_message = "Designing ADPCM codebook for %s" % src.short_path,
            tools = [ctx.executable._tabledesign],
            command = "%s %s >%s" % (
                ctx.executable._tabledesign.path,
                src.path,
                out_table.path,
            ),
        )
        out_adpcm = ctx.actions.declare_file(name + ".adpcm.aifc")
        ctx.actions.run(
            outputs = [out_adpcm],
            inputs = [src, out_table],
            progress_message = "Encoding ADPCM file %s" % out_adpcm.short_path,
            executable = ctx.executable._vadpcm_enc,
            arguments = [
                "-c",
                out_table.path,
                src.path,
                out_adpcm.path,
            ],
        )
        outputs.append(out_adpcm)
        out_decoded = ctx.actions.declare_file(name + ".dec.aiff")
        ctx.actions.run(
            outputs = [out_decoded],
            inputs = [out_adpcm],
            progress_message = "Decoding ADPCM file %s" % out_adpcm.short_path,
            executable = ctx.executable._vadpcm_dec,
            arguments = [
                out_adpcm.path,
                out_decoded.path,
            ],
        )
        outputs.append(out_decoded)
    return [DefaultInfo(files = depset(outputs))]

audio_tracks = rule(
    implementation = _audio_tracks_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            mandatory = True,
        ),
        "rate": attr.string(
            mandatory = True,
        ),
        "_flac": attr.label(
            default = Label("@tools//:flac"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
        "_sox": attr.label(
            default = Label("@tools//:sox"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
        "_tabledesign": attr.label(
            default = Label("//tools/audio/tabledesign"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
        "_vadpcm_enc": attr.label(
            default = Label("//tools/audio/encode:vadpcm_enc"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
        "_vadpcm_dec": attr.label(
            default = Label("//tools/audio/encode:vadpcm_dec"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
