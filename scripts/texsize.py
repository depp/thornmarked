import sys

def die(*msg):
    print('Error:', *msg, file=sys.stderr)
    raise SystemExit(1)

def main(argv):
    try:
        xs, ys = argv
    except ValueError:
        die('Usage: texsize.py <width> <height>')
    x = int(xs)
    y = int(ys)
    size = 0
    while x and y:
        stride = ((x + 15) & ~15) >> 1
        size += stride * y
        print('{:04}    {}x{}'.format(size, x, y))
        x >>= 1
        y >>= 1
    print('{:04}'.format(size))

if __name__ == '__main__':
    main(sys.argv[1:])
