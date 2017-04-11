#!/usr/bin/env python -u

import os
import os.path
import subprocess
import re
import shutil
import sys
import tarfile
import xml.etree.ElementTree as ET
import json
import tempfile

# These are the Anki modules/packages:
import binary_conversion
import validate_anim_data


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
  """
  Given the path to a directory that contains .tar files and a list
  of file types, eg. [".json", ".png"], this function will unpack
  the given file types from all the .tar files.  If the optional
  'put_in_subdir' input argument is set to True, then the files are
  unpacked into a sub-directory named after the .tar file.
  """
  for (dir_path, dir_names, file_names) in os.walk(extract_dir):

    # Generate list of all .tar files in/under the directory provided by the caller (extract_dir)
    all_files = map(lambda x: os.path.join(dir_path, x), file_names)
    tar_files = [a_file for a_file in all_files if a_file.endswith('.tar')]

    if tar_files and not put_in_subdir:
      # If we have any .tar files to unpack and they will NOT be unpacked into a sub-directory,
      # then first clean up existing files that may conflict with what will be unpacked. For
      # example, if a .tar file previously contained foo.json and it now contains bar.json, we
      # don't want foo.json lingering from a previous unpacking.
      # PS, if the .tar files WILL be unpacked into a sub-directory, we don't need to do any cleanup
      # here because unpack_tarball() will first delete the sub-directory if it already exists.
      file_types_to_cleanup = file_types + [binary_conversion.BIN_FILE_EXT]
      delete_files_from_dir(file_types_to_cleanup, dir_path, file_names)

    for tar_file in tar_files:
      unpack_tarball(tar_file, file_types, put_in_subdir)
      # No need to remove the tar file here after unpacking it - all tar files are
      # deleted from cozmo_resources/assets/ on the device by Builder.cs

    if put_in_subdir:
      # If we are extracting tar files into subdirs, don't recurse into those subdirs.
      break


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


def delete_files_from_dir(file_types, dir_path, file_names):
  delete_count = 0
  file_types = [str(x) for x in file_types]
  #print("Deleting all %s files from %s" % (file_types, dir_path))
  for file_name in file_names:
    file_ext = str(os.path.splitext(file_name)[1])
    if file_ext in file_types:
      os.remove(os.path.join(dir_path, file_name))
      delete_count += 1
  #print("Deleted %s files of types %s" % (delete_count, file_types))
  return delete_count


def get_file_stats(which_dir):
  file_stats = {}
  for (dir_path, dir_names, file_names) in os.walk(which_dir):
    for file_name in file_names:
      file_ext = str(os.path.splitext(file_name)[1])
      if file_ext not in file_stats:
        file_stats[file_ext] = 1
      else:
        file_stats[file_ext] += 1
  return file_stats


def convert_json_to_binary(json_files, bin_name, dest_dir, flatc_dir):
    tmp_json_files = []
    tmp_dir = tempfile.mkdtemp()
    for json_file in json_files:
        json_dest = os.path.join(tmp_dir, os.path.basename(json_file))
        shutil.move(json_file, json_dest)
        tmp_json_files.append(json_dest)
    bin_name = bin_name.lower()
    try:
        bin_file = binary_conversion.main(tmp_json_files, bin_name, flatc_dir)
    except StandardError, e:
        print("%s: %s" % (type(e).__name__, e.message))
        # If binary conversion failed, use the json files...
        for json_file in tmp_json_files:
            json_dest = os.path.join(dest_dir, os.path.basename(json_file))
            shutil.move(json_file, json_dest)
            print("Restored %s" % json_dest)
    else:
        bin_dest = os.path.join(dest_dir, bin_name)
        shutil.move(bin_file, bin_dest)


def unpack_tarball(tar_file, file_types=[], put_in_subdir=False, add_metadata=False, convert_to_binary=True):

  # TODO: Set 'add_metadata' to True when we are ready to start inserting
  # metadata into the JSON files to indicate which .tar file and version
  # the data came from.

  # If this function should convert .json data to binary, that conversion will be done with "flatc",
  # so we need to specify the path to the directory that contains that FlatBuffers tool. See
  # https://google.github.io/flatbuffers/flatbuffers_guide_using_schema_compiler.html for additional
  # info about the "flatc" schema compiler.
  flatc_dir = os.path.join(DEPENDENCY_LOCATION, 'coretech_external', 'flatbuffers', 'ios', 'Release')

  # Set the destination directory where the contents of the .tar file will be unpacked.
  dest_dir = os.path.dirname(tar_file)
  if put_in_subdir:
    subdir = os.path.splitext(os.path.basename(tar_file))[0]
    dest_dir = os.path.join(dest_dir, subdir)
    if os.path.isdir(dest_dir):
      # If the destination sub-directory already exists, get rid of it.
      shutil.rmtree(dest_dir)

  tar_file_rev = get_svn_file_rev(tar_file)

  try:
    tar = tarfile.open(tar_file)
  except tarfile.ReadError, e:
    raise RuntimeError("%s: %s" % (e, tar_file))

  if file_types:
    tar_members = _get_specific_members(tar, file_types)
    #print("Unpacking %s (version %s) (%s files)" % (tar_file, tar_file_rev, len(tar_members)))
    tar.extractall(dest_dir, members=tar_members)
    tar.close()
    if ".json" in file_types:
      json_files = [tar_info.name for tar_info in tar_members if tar_info.name.endswith(".json")]
      json_files = map(lambda x: os.path.join(dest_dir, x), json_files)
      if json_files:
        if add_metadata:
          for json_file in json_files:
            insert_source_rev_data_into_json(tar_file, tar_file_rev, json_file)
        if convert_to_binary:
          bin_name = os.path.splitext(os.path.basename(tar_file))[0] + binary_conversion.BIN_FILE_EXT
          convert_json_to_binary(json_files, bin_name, dest_dir, flatc_dir)
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

    Returns: None
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
                    except EnvironmentError, e:
                        print("Failed to unpack one or more tar files in [%s] because: %s" % (subdir, e))
                        print(stale_warning)
                    file_stats = get_file_stats(subdir)
                    print("After unpacking tar files, '%s' contains the following files: %s"
                          % (os.path.basename(subdir), file_stats))

        else:
            print "{0} does not need to be updated.  Current {1} revision at {2} ".format(repo, tool, l_rev)


def git_package(git_dict):
    # WIP.  Should this function raise a NotImplementedError exception until it is completed?
    print("The git_package() function has NOT been implemented yet")


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

        combined_url = "{0}/repository/download/{1}/{2}/{3}_{4}.{5}".format(root_url, build_type_id, version, package_name, version, ext)
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


def extract_dependencies(version_file, location=RELATIVE_EXTERNALS_DIR, validate_assets=True):
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
    if validate_assets:
        validate_anim_data.check_audio_events_all_anims(location)


def json_parser(version_file):
    """
    This function is used to parse the "DEPS" file and pull in
    any external dependencies.

    NOTE: This should pull in TeamCity dependencies, eg. CoreTech External,
    before SVN dependencies, eg. cozmo-assets, because the latter may
    depend on the former, eg. the Cozmo build uses FlatBuffers from
    CoreTech External to process animation data from cozmo-assets.

    Returns:
        object: Null

    Args:
        version_file: path
    """
    if os.path.isfile(version_file):
        with open(version_file, mode="r") as file_obj:
            djson = json.load(file_obj)
            if "teamcity" in djson:
                teamcity_package(djson["teamcity"])
            if "svn" in djson:
                svn_package(djson["svn"])
            if "git" in djson:
                git_package(djson["git"])
    else:
        sys.exit("ERROR: %s does not exist" % version_file)

def update_teamcity_version(version_file, teamcity_builds):
    """
    Update version entries of teamcity builds for dependencies.

    :rtype: Null

    Args:
        version_file: path

        teamcity_builds: dict
    """
    assert isinstance(teamcity_builds, dict)
    if os.path.isfile(version_file):
        djson = dict()
        with open(version_file, mode="r") as file_obj:
            djson = json.load(file_obj)
            assert isinstance(djson, dict)
            if "teamcity" in djson:
                tc_dict = djson["teamcity"]
                for build in tc_dict["builds"]:
                    if build in teamcity_builds:
                        djson["teamcity"]["builds"][build]["version"] = teamcity_builds[build]
            else:
                exit()
        with open(version_file, mode="w") as file_obj:
            file_obj.write(json.dumps(djson, sort_keys=True, indent=4, separators=(',', ': ')))


###############
# ENTRY POINT #
###############


def main():
    """Main should only be called for debugging."""
    extract_dependencies(RELATIVE_DEPS_FILE, RELATIVE_EXTERNALS_DIR)


if __name__ == '__main__':
    main()


