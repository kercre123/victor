#!/usr/bin/env python2

# Copy metadata from source tree

import sys
import os
import shutil

# Expected args list [root directory of source, destination to copy source tree]
def main(args):

    if not len(args) == 2:
        print('ERROR - bundle_metadata_products.py invalid input')
        return 1

    source = args[0]
    destination = args[1]
    if os.path.exists(destination):
        shutil.rmtree(destination)
    shutil.copytree(source, destination, ignore=shutil.ignore_patterns('*.wem','*.bnk'))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
