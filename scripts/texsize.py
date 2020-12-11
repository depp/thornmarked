import sys

def die(*msg):
    print('Error:', *msg, file=sys.stderr)
    raise SystemExit(1)

def main(argv):
    try:
        bitss, xs, ys = argv
    except ValueError:
        die('Usage: texsize.py <bits> <width> <height>')
    bits = int(bitss)
    x = int(xs)
    y = int(ys)
    size = 0
    while x and y:
        print('{:4}    {}x{}'.format(size, x, y))
        stride = (x * bits + 63) >> 6
        size += stride * y
        x >>= 1
        y >>= 1
    print('{:4}'.format(size))
    print('Bytes:', size << 3)

if __name__ == '__main__':
    main(sys.argv[1:])
