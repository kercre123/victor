#!/usr/bin/env python -u

import os
import os.path
import subprocess
import json
import re
import shutil

DEPENDENCY_LOCATION = ""
VERBOSE = True
# Replace True for name of log file if desired.
REPORT_ERRORS = True


def is_tool(name):
    """
    Test if application exists. Equivalent to which.
    stolen from http://stackoverflow.com/questions/11210104/check-if-a-program-exists-from-a-python-script

    Args:
        name: string

    Returns: bool

    """
    try:
        devnull = open(os.devnull)
        subprocess.Popen([name], stdout=devnull, stderr=devnull).communicate()
    except OSError as e:
        if e.errno == os.errno.ENOENT:
            return False
    return True


def is_up(url_string):
    import urllib2
    try:
        urllib2.urlopen(url_string)
        return True
    except urllib2.HTTPError, e:
        if e.code == 401:
        # Authentication error site is up and requires login.
            return True
        print(e.code)
        return False
    except urllib2.URLError, e:
        return False


def svn_package(svn_dict):
    """

    Args:
        svn_dict: dict

    Returns:

    """
    tool = "svn"
    ptool = "tar"
    ptool_options = ['-x', '-z', '-f']
    assert is_tool(tool)
    assert is_tool(ptool)
    assert isinstance(svn_dict, dict)
    root_url = svn_dict.get("root_url", "undefined_url")
    password = svn_dict.get("pwd", "")
    repos = svn_dict.get("repo_names", "")
    user = svn_dict.get("default_usr", "undefined")
    cred = ['--username', user, '--password', password, '--no-auth-cache', '--non-interactive', '--trust-server-cert']

    if not is_up(root_url):
        print "{0} is not available.  Please check your internet connection.".format(root_url)
        return

    for repo in repos:
        r_rev = repos[repo].get("version", "head")
        pk_name = repos[repo].get("package_name", "STUB")
        branch = repos[repo].get("branch", "trunk")
        url = os.path.join(root_url, repo, branch)
        loc = os.path.join(DEPENDENCY_LOCATION, repo)
        package = os.path.join(loc, pk_name)
        checkout = [tool, 'checkout', '-r', '{0}'.format(r_rev)] + cred + [url, loc]
        cleanup = [tool, 'status', '--no-ignore'] + cred + [loc]
        get_rev = [tool, "info"] + cred + [loc]
        unpack = [ptool] + ptool_options + [package, loc]

        if os.path.isdir(loc):
            pipe = subprocess.Popen(get_rev, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            svn_info, err = pipe.communicate()
            l_rev = re.search(r'Revision: ([0-9]+)', svn_info).group(1)
            try:
                l_rev = int(l_rev)
            except ValueError:
                l_rev = 0
                print " was created with a different svn."
                print "Clearing out directory before getting a fresh copy."

                shutil.rmtree(loc)
        else:
            l_rev = 0
        if r_rev == "head" or l_rev != r_rev:
            print "Checking out {0}".format(repo)
            pipe = subprocess.Popen(checkout, stdout=subprocess.PIPE, stderr= subprocess.PIPE)
            successful, err = pipe.communicate()
            if '' == err:
                print successful
                # Equivalent to a git clean
                pipe = subprocess.Popen(cleanup, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                extract_message, error = pipe.communicate()
                if '' != extract_message:
                    last_column = re.compile(r'\S+\s+(\S+)')
                    unversioned_files = last_column.findall(extract_message)
                    for a_file in unversioned_files:
                        if os.path.isdir(a_file):
                            shutil.rmtree(a_file)
                        elif os.path.isfile(a_file):
                            os.remove(a_file)

                if os.path.isfile(package):
                    # call waits for the result.  Moving on to the next checkout doesnt need this to finish.
                    subprocess.Popen(unpack, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            else:
                print "Error in checking out {0}".format(repo)
                print(err)
        else:
            print "{0} does not need to be updated.  Current {1} revision at {2} ".format(repo, tool, l_rev)


def git_package(git_dict):
    # WIP
    print("git stub")


def teamcity_package(tc_dict):
    # WIP
    print("teamcity stub")


def extract_dependencies(version_file, location):
    """
    Entry point to starting the dependency extraction.

    :rtype: Null

    Args:
        version_file: path

        location: path
    """
    global DEPENDENCY_LOCATION
    DEPENDENCY_LOCATION = location
    if not os.path.isdir(location):
        os.mkdir(location)
    json_parser(version_file)


def json_parser(version_file):
    """

    Returns:
        object: Null

    Args:
        version_file: path
    """
    if os.path.isfile(version_file):

        with open(version_file, mode="r") as file_obj:
            djson = json.load(file_obj)
            if "svn" in djson:
                svn_package(djson["svn"])
            if "teamcity" in djson:
                djson["teamcity"]
            if "git" in djson:
                djson["git"]
    else:
        print "ERROR: {0} does not exist", version_file


###############
# ENTRY POINT #
###############


def main():
    """Main should only be called for debugging."""
    extract_dependencies("../../DEPS", "../../EXTERNALS")


if __name__ == '__main__':
    main()
