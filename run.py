#!/usr/bin/env python3
import argparse
import json
import os
import pathlib
import re
import shlex
import shutil
import subprocess
import sys
import urllib.parse

NTSC_TARGET = '//game:Thornmarked_NTSC'
PAL_TARGET = '//game:Thornmarked_PAL'
EXTRA_PATHS = [
    '/usr/games',
]

SRCDIR = pathlib.Path(__file__).resolve().parent

def die(*msg):
    print('Error:', *msg, file=sys.stderr)
    raise SystemExit(1)

VERBOSE = False

def log(*msg):
    if VERBOSE:
        print(*msg, file=sys.stderr)

def find_program(name, *, extra_paths=[]):
    """Return the path to a program."""
    for path in [*extra_paths, *os.get_exec_path()]:
        exe = pathlib.Path(path, name)
        if os.access(exe, os.X_OK):
            return exe
    die('could not find program:', repr(name))
    raise SystemExit(1)

def run(args, *, exec=False):
    strargs = [str(arg) for arg in args]
    print(*[shlex.quote(arg) for arg in strargs], file=sys.stderr)
    if exec:
        os.execvp(args[0], args)
    proc = subprocess.run(args)
    if proc.returncode:
        die('Command failed:', args[0])

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
    rmmods = [mod for mod in ['ftdi_sio', 'usbserial'] if mod in mods]
    if rmmods:
        print('Modules are loaded which interfere with UNFLoader:', *rmmods)
        if confirm('Unload these modules (sudo rmmod)? [y/n] '):
            subprocess.run(['sudo', 'rmmod', *rmmods], check=True)
    try:
        run(['sudo', 'UNFLoader', '-r', rom])
    except subprocess.CalledProcessError as ex:
        print(ex, file=sys.stderr)
        raise SystemExit(1)
    finally:
        subprocess.call(['stty', 'sane'])

def cen64(rom, args):
    exe = find_program('cen64', extra_paths=EXTRA_PATHS)
    if args.region == 'ntsc':
        pifdata = SRCDIR / 'sdk/pifdata.bin'
    elif args.region == 'pal':
        pifdata = SRCDIR / 'sdk/pifdatapal.bin'
    else:
        die('unknown region')
    run([exe, pifdata, rom, *args.emulator_opts], exec=True)

def mame(rom, args):
    exe = find_program('mame', extra_paths=EXTRA_PATHS)
    run([exe, 'n64', '-cart', rom, *args.emulator_opts], exec=True)

def mupen64(rom, args):
    exe = find_program('mupen64plus', extra_paths=EXTRA_PATHS)
    run([exe, rom, *args.emulator_opts], exec=True)

def list_artifacts():
    dir = pathlib.Path.home() / 'Documents/Artifacts'
    find_revision = re.compile(r'\.r(\d+)\.')
    for item in dir.iterdir():
        m = find_revision.search(item.name)
        if m is not None:
            item_rev = int(m.group(1), 10)
            yield item_rev, item

def find_artifact(revision):
    matches = []
    has_artifact = False
    for item_rev, item in list_artifacts():
        has_artifact = True
        if item_rev == revision:
            matches.append(item)
    if not has_artifact:
        die('could not find any artifacts in directory:', dir)
    if not matches:
        die('could not find artifact with revision:', revision)
    if len(matches) > 1:
        die('multiple artifacts with revision:', revision)
    return matches[0]

HAVE_TEMP_DIR = False
def get_temp_dir():
    """Get the temporary directory, clearing it if it hasn't been used yet."""
    global CLEARED_TEMP_DIR
    temp = SRCDIR / 'temp'
    if not HAVE_TEMP_DIR:
        temp.mkdir(exist_ok=True)
        for item in temp.iterdir():
            item.unlink()
    return temp

def extract_artifact(artifact):
    temp = get_temp_dir()
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

def image_from_binary(binary):
    """Get a ROM image from the given ELF binary."""
    temp = get_temp_dir()
    binary_copy = temp / binary.name
    shutil.copy(binary, binary_copy)
    image = temp / (binary.name + '.n64')
    run([
        'bazel', 'run', '-c', 'opt', '//tools/makemask', '--',
        '-program', binary_copy,
        '-output', image,
        '-bootcode', SRCDIR / 'sdk/boot6102.bin',
    ])
    return image

def get_image(args):
    """Return a path to the image to run."""
    # From -image:
    if args.image is not None:
        return pathlib.Path(args.image)

    # From -binary:
    if args.binary is not None:
        return image_from_binary(pathlib.Path(args.binary))

    # From -artifact or -revision:
    artifact = args.artifact
    if args.revision is not None:
        artifact = find_artifact(args.revision)
    if artifact is not None:
        return extract_artifact(artifact)

    # From -target:
    target = args.target
    if target is None:
        if args.region == 'ntsc':
            target = NTSC_TARGET
        elif args.region == 'pal':
            target = PAL_TARGET
        else:
            die('unknown region')
    temp = get_temp_dir()
    args = [
        'bazel', 'build',
        '--platforms=//n64',
        '--compilation_mode=' + args.compilation_mode,
        '--build_event_json_file=temp/build.json',
        target,
    ]
    run(args)
    cevent = None
    with open(temp/'build.json') as fp:
        for line in fp:
            item = json.loads(line)
            if 'targetCompleted' in item['id']:
                cevent = item
    if cevent is None:
        die('no targetCompleted event found')
    completed = cevent['completed']
    if not completed['success']:
        die('success = false')
    output = completed['importantOutput']
    if not output:
        die('no output in targetCompleted event')
    if len(output) != 1:
        if VERBOSE:
            json.dump(output, sys.stderr, indent='  ')
            sys.stderr.write('\n')
        die('target has multiple outputs')
    uri = urllib.parse.urlparse(output[0]['uri'])
    if uri.scheme != 'file':
        die('output URI is not a file:', uri.scheme)
    outfile = pathlib.Path(uri.path)
    if outfile.suffix != '.n64':
        return image_from_binary(outfile)
    return outfile

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
    p.add_argument('-verbose', '-v', action='store_true',
        help='verbose output')
    g = p.add_mutually_exclusive_group()
    g.add_argument('-hardware', action='store_true',
        help='use UNFLoader to write the ROM image to a flashcart')
    g.add_argument('-emulator', choices=emulators, default='cen64',
        help='emulator to run game with')
    p.add_argument('-eopt', action='append', dest='emulator_opts',
        default=[], help='additional options to pass emulator')
    p.add_argument('-bopt', action='append', dest='bazel_opts',
        default=[], help='additional options to pass Bazel')
    p.add_argument('-compilation-mode', '-c', default='fastbuild',
        help='Bazel compilation mode')
    p.add_argument('-no-run', '-n', action='store_true',
        help='do not run, just show information about artifact')
    p.add_argument('-list', '-l', action='store_true',
        help='list available artifacts')
    g = p.add_mutually_exclusive_group()
    g.add_argument('-artifact',
        help='run ROM from the given artifact archive')
    g.add_argument('-revision', '-r', type=int,
        help='run rom from artifact with the given revision')
    g.add_argument('-image', '-i', help='run a specific ROM image')
    g.add_argument('-binary', '-b', help='run a specific ELF binary')
    g.add_argument('-target', '-t', help='Bazel target to run')
    g = p.add_mutually_exclusive_group()
    g.add_argument('-ntsc', action='store_const',
        dest='region', default='ntsc', const='ntsc', help='run NSTC version')
    g.add_argument('-pal', action='store_const',
        dest='region', const='pal', help='run PAL version')
    args = p.parse_args(argv)
    global VERBOSE
    VERBOSE = args.verbose

    if args.list:
        for row in sorted(list_artifacts()):
            print('{0[0]:4d}  {0[1].name}'.format(row))
        return

    image = get_image(args)

    if args.no_run:
        print('ROM Image:', image)
        return
    if args.hardware:
        hardware(image, args)
    else:
        emulators[args.emulator](image, args)

if __name__ == '__main__':
    main(sys.argv[1:])
