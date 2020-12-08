def _audio_tracks_impl(ctx):
    audioconvert = ctx.executable._audioconvert
    sox = ctx.executable._sox
    flac = ctx.executable._flac
    tabledesign = ctx.executable._tabledesign
    vadpcm_enc = ctx.executable._vadpcm_enc
    vadpcm_dec = ctx.executable._vadpcm_dec

    # List metadata and audio files.
    metadata = {}
    audio = []
    for src in ctx.files.srcs:
        if src.basename.endswith(".json"):
            metadata[src.short_path[:-5]] = src
        else:
            audio.append(src)
    outputs = []
    for src in audio:
        name = src.basename
        idx = name.rfind(".")
        ext = ""
        if idx >= 0:
            ext = name[idx:]
            name = name[:idx]
        mkey = src.short_path[:idx - len(src.basename)]
        msrc = metadata.pop(mkey, None)

        # Convert to correct rate, apply metadata.
        out_aifc = ctx.actions.declare_file(name + ".pcm.aifc")
        inputs = [src, sox]
        arguments = [
            "-input=" + src.path,
            "-output=" + out_aifc.path,
            "-rate=" + ctx.attr.rate,
            "-sox=" + sox.path,
        ]
        if msrc:
            inputs.append(msrc)
            arguments.append("-metadata=" + msrc.path)
        if ext == ".flac":
            inputs.append(flac)
            arguments.append("-flac=" + flac.path)
        ctx.actions.run(
            outputs = [out_aifc],
            inputs = inputs,
            progress_message = "Converting %s" % src.short_path,
            executable = audioconvert,
            arguments = arguments,
        )

        # Generate codebook.
        out_table = ctx.actions.declare_file(name + ".table.txt")
        ctx.actions.run_shell(
            outputs = [out_table],
            inputs = [out_aifc],
            progress_message = "Designing ADPCM codebook for %s" % src.short_path,
            tools = [tabledesign],
            command = "%s %s >%s" % (
                tabledesign.path,
                out_aifc.path,
                out_table.path,
            ),
        )

        # Encode VADPCM.
        out_adpcm = ctx.actions.declare_file(name + ".adpcm.aifc")
        ctx.actions.run(
            outputs = [out_adpcm],
            inputs = [out_aifc, out_table],
            progress_message = "Encoding ADPCM file %s" % out_adpcm.short_path,
            executable = vadpcm_enc,
            arguments = [
                "-c",
                out_table.path,
                out_aifc.path,
                out_adpcm.path,
            ],
        )
        outputs.append(out_adpcm)

        # Decode back to PCM.
        out_decoded = ctx.actions.declare_file(name + ".dec.aiff")
        ctx.actions.run(
            outputs = [out_decoded],
            inputs = [out_adpcm],
            progress_message = "Decoding ADPCM file %s" % out_adpcm.short_path,
            executable = vadpcm_dec,
            arguments = [
                out_adpcm.path,
                out_decoded.path,
            ],
        )
        outputs.append(out_decoded)

    for src in metadata.values():
        fail("Unused input: %s" % src)
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
        "_audioconvert": attr.label(
            default = Label("//tools/audioconvert"),
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
