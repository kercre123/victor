#!/usr/bin/env python -u

import os
import os.path
import subprocess
import json
import re
import shutil
import sys
import tarfile
import xml.etree.ElementTree as ET
import json

VERBOSE = True
# Replace True for name of log file if desired.
REPORT_ERRORS = True
SVN_INFO_CMD = "svn info %s %s --xml"
SVN_CRED = "--username %s --password %s --no-auth-cache --non-interactive --trust-server-cert"
RELATIVE_DEPS_FILE = "../../DEPS"
RELATIVE_EXTERNALS_DIR = "../../EXTERNALS"
DIFF_BRANCH_MSG = "is already a working copy for a different URL"

# Most animation tar files in SVN are packages of JSON files that should be unpacked in the root
# "animations" directory, but facial animation tar files (packages of PNG files) should be unpacked
# in a subdirectory of the root "faceAnimations" directory. The following list indicates the groups
# of tar files that should be unpacked in a subdirectory, which is named after the tar file.
# TODO: Put this info (unpack files in root directory or subdirectory) somewhere in the DEPS file.
UNPACK_INTO_SUBDIR = ["faceAnimations"]


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


def _extract_files_from_tar(extract_dir, file_types, put_in_subdir=False):
  for (dir_path, dir_names, file_names) in os.walk(extract_dir):
    all_files = map(lambda x: os.path.join(dir_path, x), file_names)
    tar_files = [a_file for a_file in all_files if a_file.endswith('.tar')]
    for tar_file in tar_files:
      unpack_tarball(tar_file, file_types, put_in_subdir)
      # No need to remove the tar file here after unpacking it - all tar files are
      # deleted from cozmo_resources/assets/ on the device by Builder.cs


def _get_specific_members(members, file_types):
  file_list = []
  for tar_info in members:
    if os.path.splitext(tar_info.name)[1] in file_types:
      file_list.append(tar_info)
  return file_list


def insert_source_rev_data_into_json(tar_file, tar_file_rev, json_file):
  #print("Inserting source file revision info into: %s" % json_file)
  metadata = {'tar_file' : tar_file,
              'tar_version' : tar_file_rev}
  metadata = {'metadata' : metadata}
  with open(json_file, 'a') as file_obj:
    file_obj.write("%s%s" % (json.dumps(metadata), os.linesep))


def unpack_tarball(tar_file, file_types=[], put_in_subdir=False, add_metadata=False):

  # TODO: Set 'add_metadata' to True when we are ready to start inserting
  # metadata into the JSON files to indicate which .tar file and version
  # the data came from.

  dest_dir = os.path.dirname(tar_file)
  if put_in_subdir:
      subdir = os.path.splitext(os.path.basename(tar_file))[0]
      dest_dir = os.path.join(dest_dir, subdir)
  try:
    tar = tarfile.open(tar_file)
  except tarfile.ReadError, e:
    raise RuntimeError("%s: %s" % (e, tar_file))
  tar_file_rev = get_svn_file_rev(tar_file)
  if file_types:
    tar_members = _get_specific_members(tar, file_types)
    #print("Unpacking %s (version %s) (%s files)" % (tar_file, tar_file_rev, len(tar_members)))
    tar.extractall(dest_dir, members=tar_members)
    if add_metadata and ".json" in file_types:
      json_files = [tar_info.name for tar_info in tar_members]
      json_files = map(lambda x: os.path.join(dest_dir, x), json_files)
      for json_file in json_files:
        insert_source_rev_data_into_json(tar_file, tar_file_rev, json_file)
  else:
    #print("Unpacking %s (version %s) (all files)" % (tar_file, tar_file_rev))
    tar.extractall(dest_dir)
  tar.close()


def get_svn_file_rev(file_from_svn, cred=''):
  """
  Given a file that was checked out from SVN, this function will
  return the revision of that file in the form of an integer. If
  the revision cannot be determined, this function returns None.
  """
  svn_info_cmd = SVN_INFO_CMD % (cred, file_from_svn)
  #print("Running: %s" % svn_info_cmd)
  p = subprocess.Popen(svn_info_cmd.split(), stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE)
  (stdout, stderr) = p.communicate()
  status = p.poll()
  if status != 0:
    return None
  root = ET.fromstring(stdout.strip())
  for entry in root.iter('entry'):
    try:
      rev = entry.attrib['revision']
    except KeyError:
      pass
    else:
      rev = int(rev)
      return rev


def svn_checkout(checkout, cleanup, unpack, package, allow_extra_files, verbose=VERBOSE):
    pipe = subprocess.Popen(checkout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    successful, err = pipe.communicate()
    status = pipe.poll()
    #print("status = %s" % status)
    if err == '' and status == 0:
        print(successful.strip())
        # Equivalent to a git clean
        pipe = subprocess.Popen(cleanup, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        extract_message, error = pipe.communicate()
        if extract_message != '' and not allow_extra_files:
            last_column = re.compile(r'\S+\s+(\S+)')
            unversioned_files = last_column.findall(extract_message)
            for a_file in unversioned_files:
                if os.path.isdir(a_file):
                    shutil.rmtree(a_file)
                elif os.path.isfile(a_file):
                    os.remove(a_file)
        if os.path.isfile(package):
            # call waits for the result.  Moving on to the next checkout doesnt need this to finish.
            pipe = subprocess.Popen(unpack, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            successful, err = pipe.communicate()
            if verbose:
                print(err)
        return ''
    else:
        return err


def svn_package(svn_dict):
    """

    Args:
        svn_dict: dict

    Returns:

    """
    tool = "svn"
    ptool = "tar"
    ptool_options = ['-v', '-x', '-z', '-f']
    assert is_tool(tool)
    assert is_tool(ptool)
    assert isinstance(svn_dict, dict)
    root_url = svn_dict.get("root_url", "undefined_url")
    password = svn_dict.get("pwd", "")
    repos = svn_dict.get("repo_names", "")
    user = svn_dict.get("default_usr", "undefined")
    cred = SVN_CRED % (user, password)
    stale_warning = "WARNING: If this build succeeds, it may contain stale external data"

    if not is_up(root_url):
        print "{0} is not available.  Please check your internet connection.".format(root_url)
        print(stale_warning)
        return

    for repo in repos:
        r_rev = repos[repo].get("version", "head")
        pk_name = repos[repo].get("package_name", "STUB")
        branch = repos[repo].get("branch", "trunk")
        url = os.path.join(root_url, repo, branch)
        loc = os.path.join(DEPENDENCY_LOCATION, repo)
        subdirs = repos[repo].get("subdirs", [])
        subdirs = map(lambda x: os.path.join(loc,x), subdirs)
        allow_extra_files = bool(repos[repo].get("allow_extra_files", False))
        extract_types = repos[repo].get("extract_types_from_tar", [])
        package = os.path.join(loc, pk_name)
        checkout = [tool, 'checkout', '-r', '{0}'.format(r_rev)] + cred.split() + [url, loc]
        cleanup = [tool, 'status', '--no-ignore'] + cred.split() + [loc]
        unpack = [ptool] + ptool_options + [package, '-C', loc]
        l_rev = 'unknown'

        for subdir in subdirs:
          if not os.path.isdir(subdir):
            l_rev = 0
            break
        if l_rev != 0 and os.path.isdir(loc):
            l_rev = get_svn_file_rev(loc, cred)
            #print("The version of [%s] is [%s]" % (loc, l_rev))
            if l_rev is None:
                l_rev = 0
                msg = "Clearing out [%s] directory before getting a fresh copy."
                if subdirs:
                    for subdir in subdirs:
                        if os.path.exists(subdir):
                            print(msg % subdir)
                            shutil.rmtree(subdir)
                elif os.path.exists(loc):
                    print(msg % loc)
                    shutil.rmtree(loc)
        else:
            l_rev = 0

        try:
            r_rev = int(r_rev)
        except ValueError:
            pass

        if r_rev == "head" or l_rev != r_rev:
            print("Checking out '{0}'".format(repo))
            err = svn_checkout(checkout, cleanup, unpack, package, allow_extra_files)
            if err:
                print("Error in checking out {0}: {1}".format(repo, err.strip()))
                if DIFF_BRANCH_MSG in err:
                    print("Clearing out [%s] directory to replace it with a fresh copy" % loc)
                    shutil.rmtree(loc)
                    print("Checking out '{0}' again".format(repo))
                    err = svn_checkout(checkout, cleanup, unpack, package, allow_extra_files)
                    if err:
                        print("Error in checking out {0}: {1}".format(repo, err.strip()))
                        print(stale_warning)
                else:
                    print(stale_warning)
            if extract_types:
                for subdir in subdirs:
                    put_in_subdir = os.path.basename(subdir) in UNPACK_INTO_SUBDIR
                    try:
                        _extract_files_from_tar(subdir, extract_types, put_in_subdir)
                    except (OSError, IOError), e:
                        print("Failed to unpack one or more tar files in [%s]" % subdir)
                        print(stale_warning)

        else:
            print "{0} does not need to be updated.  Current {1} revision at {2} ".format(repo, tool, l_rev)


def git_package(git_dict):
    # WIP
    print("git stub")


def teamcity_package(tc_dict):
    tool = "curl"
    ptool = "tar"
    ptool_options = ['-v', '-x', '-z', '-f']
    assert is_tool(tool)
    assert is_tool(ptool)
    assert isinstance(tc_dict, dict)
    root_url = tc_dict.get("root_url", "undefined_url")
    password = tc_dict.get("pwd", "")
    user = tc_dict.get("default_usr", "undefined")
    builds = tc_dict.get("builds", "undefined")

    if not is_up(root_url):
        print "WARNING {0} is not available.  Please check your internet connection.".format(root_url)
        return

    for build in builds:
        new_version = True
        build_id = builds[build].get("build_id", "undefined")
        version = builds[build].get("version", "undefined")
        build_type_id = builds[build].get("build_type_id", "undefined")
        package_name = builds[build].get("package_name", "undefined")
        ext = builds[build].get("extension", "undefined")
        # TODO:  Switch parameters and compression tool based on extension.

        loc = os.path.join(DEPENDENCY_LOCATION, build)
        unpackage_location = loc
        version_file = os.path.join(loc, "VERSION")
        package = package_name + '.' + ext
        dist = os.path.join(DEPENDENCY_LOCATION, package)
        #This is different from svn_package because upackage_location instead of loc (it can't be assumed that the package has a folder.
        unpack = [ptool] + ptool_options + [dist, '-C', unpackage_location]

        combined_url = "{0}/repository/download/{1}/{2}:id/{3}_{4}.{5}".format(root_url, build_type_id, build_id, package_name, version, ext)
        pull_down = [tool, '--user', "{0}:{1}".format(user, password), '-f', combined_url, '-o', dist]
        if VERBOSE:
            pull_down += ['-v']

        if not is_up(combined_url):
            print "WARNING {0} is not available.  Cannot verify {1}.".format(combined_url, build)
            return

        if not os.path.exists(loc):
            os.makedirs(loc)
            vfile = open(version_file, mode="w")
            vfile.write(str(version))
            vfile.close()
        else:
            if os.path.isfile(version_file):
                with open(version_file, 'r') as vfile:
                    read_data = vfile.read().strip()
                    if int(read_data) == int(version):
                        # Prevent anymore work if files. are the same.
                        new_version = False
            else:
                #Should only get here if VERSION file was manually removed.
                if VERBOSE:
                    print "Generating version file."
                vfile = open(version_file, mode="w")
                vfile.write(str(version))
                vfile.close()
                new_version = False

        if new_version and os.path.isdir(loc):
            pipe = subprocess.Popen(pull_down, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            curl_info, err = pipe.communicate()
            status = pipe.poll()

            if status != 0:
                print err
                if os.path.isfile(dist):
                    os.remove(dist)
                print "ERROR {0}ing {1}.  Please try again.".format(tool, package)
            else:
                print "{0} Downloaded.  New version {1} ".format(build.title(), version)
        else:
            print "{0} does not need to be updated.  Current version {1}".format(build.title(), version)
        if os.path.isfile(dist):
            if os.path.isdir(unpackage_location):
                shutil.rmtree(unpackage_location)
                os.mkdir(unpackage_location)
            else:
                os.mkdir(unpackage_location)
            pipe = subprocess.Popen(unpack, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            successful, err = pipe.communicate()
            os.remove(dist)
            if VERBOSE:
                print err


def extract_dependencies(version_file, location=RELATIVE_EXTERNALS_DIR):
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
        os.makedirs(location)
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
                teamcity_package(djson["teamcity"])
            if "git" in djson:
                djson["git"]
    else:
        sys.exit("ERROR: %s does not exist" % version_file)


###############
# ENTRY POINT #
###############


def main():
    """Main should only be called for debugging."""
    extract_dependencies(RELATIVE_DEPS_FILE, RELATIVE_EXTERNALS_DIR)


if __name__ == '__main__':
    main()
