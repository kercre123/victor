#!/usr/bin/env python3

import argparse
import collections
import os.path
import subprocess
import sys

def CopyAssets(source_dest_files, source_dir, output_dir, cmake=None):
    def get_contents(file):
        with open(file) as f:
            return f.read()

    copies = list()
    for pair in source_dest_files:
        sources = get_contents(pair[0]).split('\n')
        dests = get_contents(pair[1]).split('\n')
        copies = copies + list(map(list, zip(sources, dests)))

    # copies is now a list of [source, dest] pairs

    # to utilize cmake's -E copy_if_different <src1> <src2> ...<srcN> <destDirectory> functionality,
    # let's make a dictionary of copies where the source and dest filenames are identical and we can just
    # key them on the dest dir
    dircopies = collections.defaultdict(list)
    for pair in copies:
        if os.path.basename(pair[0]) == os.path.basename(pair[1]):
            destdir = os.path.dirname(pair[1])
            dircopies[destdir].append(pair[0])
            # signal that this pair should be removed from copies since it's now in dircopies
            pair[0] = None

    # copies is just the leftovers from everything that didn't fit in dircopies
    copies = [x for x in copies if x[0]]

    if cmake is None:
        cmake = 'cmake'

    # stuff in dircopies just needs one execution per dest folder
    for (destdir, sources) in dircopies.items():
        srcs = [os.path.join(source_dir, x) for x in sources]
        dst = os.path.join(output_dir, destdir)
        if not os.path.exists(dst):
            os.makedirs(dst)
        subprocess.call([cmake, '-E', 'copy_if_different'] + srcs + [dst])

    # stuff in copies (probably empty, usually?) has to be individually copied
    for pair in copies:
        src = os.path.join(source_dir, pair[0])
        dst = os.path.join(output_dir, pair[1])
        subprocess.call([cmake, '-E', 'copy_if_different', src, dst])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='copies files in a source list to a destination list')
    parser.add_argument('--srcdst', nargs=2, required=True, action='append',
        help='a pair of files, first a source list, then a destination list; '
            'each line in the source list maps to a line in the destination list')
    parser.add_argument('--cmake', action='store')
    parser.add_argument('--output_dir', action='store', required=True)
    parser.add_argument('--source_dir', action='store', required=True)

    options, args = parser.parse_known_args(sys.argv)
    CopyAssets(options.srcdst, options.source_dir, options.output_dir, options.cmake)
