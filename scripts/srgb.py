import math
import re
import sys

def read_color(s):
    s = s.strip()
    m = re.fullmatch(r'rgb\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)', s)
    if m:
        return [int(x) for x in m.groups()]
    m = re.fullmatch('#([0-9a-fA-F]{3})', s)
    if m:
        return [0x11 * int(x,16) for x in m.group(1)]
    m = re.fullmatch('#([0-9a-fA-F]{6})', s)
    if m:
        h = m.group(1)
        return [int(h[i:i+2],16) for i in range(0, 6, 2)]
    raise ValueError('invalid color')

def to_linear(rgb):
    r = []
    for x in rgb:
        xf = x / 255
        if xf < 0.04045:
            xf /= 19.92
        else:
            xf = ((xf + 0.055) / 1.055)**2.4
        x = max(0, min(255, math.trunc(256 * xf)))
        r.append(x)
    return r

def main(argv):
    for arg in argv:
        rgb = read_color(arg)
        print(rgb, '=>', to_linear(rgb))

if __name__ == '__main__':
    main(sys.argv[1:])
