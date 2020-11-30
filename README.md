# Thornmarked

This is an attempt to make a game for Nintendo 64 by Dietrich Epp and Alastair Low for the [2020 N64brew Game Jam](https://www.youtube.com/watch?v=iOfIlATiGR4).

The game jam deadline is Sunday, December 13, 2020.

## Building

Building requires the following prerequisites:

- [Bazel](https://bazel.build/) 3.7.0
- [Pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
- [SoX](http://sox.sourceforge.net/)
- [FLAC](https://xiph.org/flac/)
- [AssImp](https://www.assimp.org/)
- [FreeType](https://www.freetype.org/)
- A MIPS toolchain installed in `/opt/n64` with the prefix `mips32-elf`

### MIPS Toolchain

See [Building GCC](https://n64brew.dev/wiki/Building_GCC).

### Debian / Ubuntu / etc.

Get a Bazel .deb package from [GitHub releases page](https://github.com/bazelbuild/bazel/releases).

```shell
sudo apt install sox flac libassimp-dev libfreetype-dev
```

### macOS

Use Homebrew.

```shell
brew install bazel sox flac assimp freetype
```
