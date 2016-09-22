#!/usr/bin/env python -u

import sys
import os
import argparse
import inspect
import time

FILE_PATH = os.path.normpath(
        os.path.abspath(os.path.realpath(os.path.dirname(inspect.getfile(inspect.currentframe())))))
BUILD_TOOLS_ROOT = os.path.normpath(
        os.path.abspath(os.path.join(FILE_PATH, "..", "..", 'tools', 'build', 'tools')))
sys.path.insert(0, BUILD_TOOLS_ROOT)
import ankibuild.ios_deploy

####################
# ARGUMENT PARSING #
####################

def parse_arguments():
    parser = argparse.ArgumentParser()

    parser.add_argument(
            '--sdk-root',
            metavar='string',
            required=True,
            help='Root location of run script and where examples live')

    parser.add_argument(
            '--sdk-dev-root',
            metavar='string',
            required=True,
            help='Location of where the the get das logs script live')

    parser.add_argument(
            '--app-artifact',
            metavar='string',
            required=True,
            help='Location of the created app which will be installed on the ios device')

    return parser.parse_args()


###############
# TEST RUNNER #
###############

class TestRunner(object):
    def __init__(self, sdkRoot, sdkDevRoot, appArtifact):
        self.sdkRoot = sdkRoot
        self.sdkDevRoot = sdkDevRoot
        self.appArtifact = appArtifact

    def run(self):
        ankibuild.ios_deploy.justlaunch(self.appArtifact)
        dasScript = self.sdkDevRoot + "/get_das_logs.py"

        #Wait for the app to start up so the SDK is running
        #We don't have an indication about that status until the SDK is up and running
        #So we must do a naive wait until we think it will be done
        time.sleep(60)
        #This will be extended to multiple script ideally
        os.system(dasScript)

###############
# ENTRY POINT #
###############

def main():
    options = parse_arguments()
    testRunner = TestRunner(options.sdk_root, options.sdk_dev_root, options.app_artifact)
    testRunner.run()

if __name__ == '__main__':
    main()
