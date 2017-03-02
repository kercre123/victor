#!/usr/bin/env python2 -u

from dependencies import update_teamcity_version
import subprocess
import os
import anki_github


def execute(*kargs):
    p = subprocess.Popen(kargs, stdout=subprocess.PIPE)
    # assert p.wait() == 0
    return p.communicate()[0].decode()


if __name__ == '__main__':
    version = os.getenv("ANKI_BUILD_VERSION")
    pr_branch = os.getenv("GIT_BRANCH")
    auth_token = os.getenv("BS_AUTH_TOKEN")
    if version is not None and pr_branch is not None and auth_token is not None:
        if pr_branch != 'master':
            # checkout out the original branch instead of the PR read-only
            branch_number = int(filter(str.isdigit, pr_branch))
            branch = anki_github.get_branch_name(auth_token, branch_number)
            print('WARNING: Developers should not check in changes to the PR at during this build.')
            execute('git', 'fetch')
            execute('git', 'checkout', '-f', branch)

        #this script could be run on master. But there should be no branch switching or committing.
        update_teamcity_version("DEPS", {'firmware': version})
        if pr_branch != 'master':
            execute('git', 'add', 'DEPS')
            execute('git', 'commit', '-m', 'Auto committing firmware from {0}'.format(branch))
            execute('git', 'push', 'origin', '{0}'.format(branch))
            #get of branch and clean up.
            execute('git', 'checkout', 'HEAD^')
            execute('git', 'branch', '-D', branch)
        else:
            print("Warning no auto committing to master")
    else:
        print("This script is not being run on a build server")
