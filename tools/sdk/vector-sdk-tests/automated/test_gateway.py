#!/usr/bin/env python3

import os
import sys
import shutil

def require_success(*args, **kwargs):
    print("> {}".format(*args))
    status = os.system(*args, **kwargs)
    if status != 0:
        sys.exit(status)

def main():
    # setup the gateway tests
    cur_dir = os.path.dirname(os.path.realpath(__file__))
    os.chdir(cur_dir)
    shutil.rmtree("env", ignore_errors=True)
    require_success("{} -m pip install virtualenv".format(sys.executable))
    require_success("{0} -m virtualenv -p {0} ./env --no-site-packages ".format(sys.executable))
    require_success("source ./env/bin/activate && ../../gateway-tests/setup.sh")
    # run the gateway tests
    # TODO: fix the deprecation warning by updating the required version of protobuf when
    # the protobuf developers fix it.
    require_success("source ./env/bin/activate && ANKI_ROBOT_SERIAL=Local python -m pytest -vs ../../gateway-tests")

if __name__ == "__main__":
    main()