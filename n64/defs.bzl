def n64_rom(name, program, data, visibility=None):
    """Create a Nintendo 64 ROM image with the given program and data."""
    native.genrule(
        name = name,
        srcs = [
            program,
            data,
            "//sdk:boot6102.bin",
        ],
        tools = [
            "//tools/makemask",
        ],
        outs = [name + ".n64"],
        cmd = ("$(execpath //tools/makemask) -output $@ -bootcode $(execpath //sdk:boot6102.bin) -program $(location %s) -pak $(location %s)" % (program, data)),
        visibility = visibility,
    )
