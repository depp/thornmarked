import argparse
import os
import subprocess
import sys

def get_duration(path):
    cmd = [
        'ffprobe',
        '-v', 'error',
        '-show_entries', 'format=duration',
        '-of', 'default=noprint_wrappers=1:nokey=1',
        path,
    ]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, check=True)
    return float(proc.stdout)

def main(argv):
    p = argparse.ArgumentParser('encode.py', allow_abbrev=False)
    p.add_argument('input', help='input file')
    p.add_argument('output', help='output file prefix')
    p.add_argument('-crf', help='CRF for x264')
    p.add_argument('-trim-start', type=float,
                   help='trim seconds from start')
    p.add_argument('-trim-end', type=float,
                   help='trim seconds from end')
    p.add_argument('-duration', type=float,
                   help='duration of clip')
    p.add_argument('-boost', action='store_true',
                   help='boost contrast')
    args = p.parse_args()

    vfilter = None
    if args.boost:
        vfilter = 'eq=gamma=1.5:contrast=1.1:saturation=1.5'

    # Create poster.
    cmd = ['ffmpeg', '-i', args.input]
    if args.trim_start is not None:
        cmd.extend(['-s', str(args.trim_start)])
    cmd.extend(['-vframes', '1', '-q:v', '2'])
    if vfilter is not None:
        cmd.extend(['-filter:v', vfilter])
    cmd.append(args.output + '.jpg')
    subprocess.run(cmd, check=True)

    # Create videos.
    duration = get_duration(args.input)
    cmd_base = [
        'ffmpeg',
        '-i', args.input,
    ]
    if args.trim_start is not None:
        cmd_base.extend(['-s', str(args.trim_start)])
    if args.trim_end is not None:
        cmd_base.extend(['-t', str(duration - args.trim_start - args.trim_end)])
    if vfilter is not None:
        cmd_base.extend(['-filter:v', vfilter])
    cmd_base.extend([
        '-pix_fmt', 'yuv420p',
        '-an',
    ])

    # Create MPEG-4 video.
    cmd = [
        *cmd_base,
        '-codec:v', 'libx264',
        '-profile:v', 'high',
        '-preset', 'slow',
    ]
    if args.crf is not None:
        cmd.extend(['-crf', str(args.crf)])
    cmd.append(args.output + '.mp4')
    subprocess.run(cmd, check=True)
    mp4_size = os.stat(args.output + '.mp4').st_size

    # Create VP9 video.
    bitrate = round(mp4_size * 8 / duration)
    print('Bitrate:', bitrate)
    cmd = [
        *cmd_base,
        '-codec:v', 'libvpx-vp9',
        '-b:v', str(bitrate),
        args.output + '.webm',
    ]
    subprocess.run(cmd, check=True)

if __name__ == '__main__':
    main(sys.argv[1:])
