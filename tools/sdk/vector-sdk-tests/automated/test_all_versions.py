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
    # setup the all versions tests
    cur_dir = os.path.dirname(os.path.realpath(__file__))
    os.chdir(cur_dir)
    shutil.rmtree("env", ignore_errors=True)
    require_success("{} -m pip install virtualenv".format(sys.executable))
    require_success("{0} -m virtualenv -p {0} ./env --no-site-packages ".format(sys.executable))
    require_success("source ./env/bin/activate && ../../gateway-tests/setup.sh")
    # Finish onboarding otherwise the sdk tests will error out
    require_success("source ./env/bin/activate && ANKI_ROBOT_SERIAL=Local python -m pytest -qs ../../gateway-tests/test_json_rpcs.py::test_onboarding_mark_complete_and_exit")
    # run the all versions tests
    require_success("{} ../../vector-python-sdk-private/tests/all_versions.py -s local -v".format(sys.executable))

if __name__ == "__main__":
    main()