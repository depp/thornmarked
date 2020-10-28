#!/usr/bin/env python3
import argparse
import os
import pathlib
import shlex
import sys

ROM = 'Thornmarked.n64'
EXTRA_PATHS = [
    '/usr/games',
]

SRCDIR = pathlib.Path(__file__).resolve().parent

def find_program(name, *, extra_paths=[]):
    """Return the path to a program."""
    for path in [*extra_paths, *os.get_exec_path()]:
        exe = pathlib.Path(path, name)
        if os.access(exe, os.X_OK):
            return exe
    print('Error: could not find program:', repr(name), file=sys.stderr)
    raise SystemExit(1)

def run(args):
    strargs = [str(arg) for arg in args]
    print(*[shlex.quote(arg) for arg in strargs], file=sys.stderr)
    os.execvp(args[0], args)

def cen64(rom, args):
    exe = find_program('cen64', extra_paths=EXTRA_PATHS)
    run([exe, SRCDIR / 'sdk/pifdata.bin', rom, *args.emulator_opts])

def mame(rom, args):
    exe = find_program('mame', extra_paths=EXTRA_PATHS)
    run([exe, 'n64', '-cart', rom, *args.emulator_opts])

def mupen64(rom, args):
    exe = find_program('mupen64plus', extra_paths=EXTRA_PATHS)
    run([exe, rom, *args.emulator_opts])

def main(argv):
    emulators = {
        'cen64': cen64,
        'mame': mame,
        'mupen': mupen64,
        'mupen64': mupen64,
    }
    p = argparse.ArgumentParser(
        prog='run.py',
        description='Run game',
        allow_abbrev=False,
    )
    p.add_argument('-emulator', choices=emulators, default='cen64',
        help='emulator to run game with')
    p.add_argument('emulator_opts', nargs='*',
        help='additional options to pass emulator')
    args = p.parse_args(argv)

    rom = pathlib.Path(SRCDIR, 'bazel-bin', 'game', ROM)
    emulators[args.emulator](rom, args)

if __name__ == '__main__':
    main(sys.argv[1:])
