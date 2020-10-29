#!/usr/bin/env python3
import argparse
import os
import pathlib
import shlex
import subprocess
import sys

ROM = 'Thornmarked.n64'
EXTRA_PATHS = [
    '/usr/games',
]

SRCDIR = pathlib.Path(__file__).resolve().parent

def die(*msg):
    print('Error:', *msg, file=sys.stderr)

def find_program(name, *, extra_paths=[]):
    """Return the path to a program."""
    for path in [*extra_paths, *os.get_exec_path()]:
        exe = pathlib.Path(path, name)
        if os.access(exe, os.X_OK):
            return exe
    die('could not find program:', repr(name))
    raise SystemExit(1)

def run(args):
    strargs = [str(arg) for arg in args]
    print(*[shlex.quote(arg) for arg in strargs], file=sys.stderr)
    os.execvp(args[0], args)

def lsmod():
    proc = subprocess.run(['lsmod'], stdout=subprocess.PIPE)
    if proc.returncode:
        die('lsmod failed')
    mods = set()
    for line in proc.stdout.splitlines():
        mod, _ = line.split(maxsplit=1)
        mod = mod.decode('ASCII')
        mods.add(mod)
    return mods

def confirm(prompt):
    while True:
        resp = input(prompt)
        resp = resp.strip().lower()
        if resp in ('y', 'yes'):
            return True
        if resp in ('n', 'no'):
            return False

def hardware(rom, args):
    mods = lsmod()
    rmmods = mods.intersection(['ftdi_sio', 'usbserial'])
    if rmmods:
        print('Modules are loaded which interfere with UNFLoader:', *rmmods)
        if confirm('Unload these modules (sudo rmmod)? [y/n] '):
            subprocess.run(['sudo', 'rmmod', *rmmods], check=True)
    run(['sudo', 'UNFLoader', '-r', rom])

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
    p.add_argument('-hardware', action='store_true',
        help='use UNFLoader to write the ROM image to a flashcart')
    p.add_argument('-emulator', choices=emulators, default='cen64',
        help='emulator to run game with')
    p.add_argument('emulator_opts', nargs='*',
        help='additional options to pass emulator')
    args = p.parse_args(argv)

    rom = pathlib.Path(SRCDIR, 'bazel-bin', 'game', ROM)
    if args.hardware:
        hardware(rom, args)
    else:
        emulators[args.emulator](rom, args)

if __name__ == '__main__':
    main(sys.argv[1:])
