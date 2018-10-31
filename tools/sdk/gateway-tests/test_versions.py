# This test file runs via pytest, and will test the json endpoints for all of the unary rpcs.
#
# This file reproduces a connection that is similar to the way the app will talk to Vector.
#

import pytest
import time

import interface_version

print("Is this before collecting...?")
time.sleep(5)

def test_shawn():
    print("Running test...")
    interface_version.v1_0_0.thing()
