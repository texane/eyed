#!/usr/bin/env python


def get_filename(filename, ext):
    toks = filename.rsplit('.', 1)
    return toks[0] + '.' + ext


def convert_file(filename, ext):
    import Image
    new_filename = get_filename(filename, ext)
    im = Image.open(filename)
    im.save(new_filename)


def convert_dir(dirname, ext):
    import os

    for dirname, dirnames, filenames in os.walk(dirname):
        for filename in filenames:
            convert_file(os.path.join(dirname, filename), ext)

    return

def main():
    import sys
    convert_file(sys.argv[1], sys.argv[2])

main()
