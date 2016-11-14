#!/usr/bin/env python2 -u

from dependencies import update_teamcity_version
import subprocess
import os

def execute(*kargs):
    p = subprocess.Popen(kargs, stdout=subprocess.PIPE)
    #assert p.wait() == 0
    return p.communicate()[0].decode()


if __name__ == '__main__':
   version = os.getenv("ANKI_BUILD_VERSION")
   branch = os.getenv("GIT_BRANCH")
   if version != None and branch != None:
        update_teamcity_version("DEPS", {'firmware': version})
        if branch != 'master':
            execute('git', 'commit','-a','-m Auto committing firmware from {0}'.format(branch))
            execute('git', 'push', 'origin', '{0}'.format(branch))
        else:
            print("Warning no auto committing to master")
   else:
        print("This script is not being run on a build server")
