#!/usr/bin/env python3
import argparse
import json
import os
import pathlib
import re
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
    raise SystemExit(1)

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

def find_artifact(revision):
    dir = pathlib.Path.home() / 'Documents/Artifacts'
    find_revision = re.compile(r'\.r(\d+)\.')
    matches = []
    has_artifact = False
    for item in dir.iterdir():
        m = find_revision.search(item.name)
        if m is not None:
            has_artifact = True
            item_rev = int(m.group(1), 10)
            if item_rev == revision:
                matches.append(item)
    if not has_artifact:
        die('could not find any artifacts in directory:', dir)
    if not matches:
        die('could not find artifact with revision:', revision)
    if len(matches) > 1:
        die('multiple artifacts with revision:', revision)
    return matches[0]

def extract_artifact(artifact):
    temp = SRCDIR / 'temp'
    temp.mkdir(exist_ok=True)
    for item in temp.iterdir():
        item.unlink()
    subprocess.run(
        ['tar', 'xf', artifact],
        cwd=temp,
        check=True,
    )
    with (temp/'INFO.json').open() as fp:
        info = json.load(fp)
    print('Revision:', info['revision'])
    print('Message:')
    for line in info['message'].splitlines():
        print('    ' + line.rstrip())
    roms = [item for item in temp.iterdir() if item.suffix == '.n64']
    if not roms:
        die('could not find ROM in artifact')
    if len(roms) > 1:
        die('multiple ROMs in artifact')
    return roms[0]

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
    p.add_argument('-artifact',
        help='run ROM from the given artifact archive')
    p.add_argument('-revision', '-r', type=int,
        help='run rom from artifact with the given revision')
    p.add_argument('-no-run', '-n', action='store_true',
        help='do not run, just show information about artifact')
    args = p.parse_args(argv)

    artifact = args.artifact
    if artifact is None and args.revision is not None:
        artifact = find_artifact(args.revision)
    if artifact is None:
        rom = pathlib.Path(SRCDIR, 'bazel-bin', 'game', ROM)
    else:
        rom = extract_artifact(artifact)
    if args.no_run:
        return
    if args.hardware:
        hardware(rom, args)
    else:
        emulators[args.emulator](rom, args)

if __name__ == '__main__':
    main(sys.argv[1:])
