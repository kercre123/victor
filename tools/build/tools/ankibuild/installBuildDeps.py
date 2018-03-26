#!/usr/bin/python

import sys
import subprocess
import argparse
import platform
import signal
import re
from distutils.version import LooseVersion, StrictVersion
from os import getenv, path, killpg, setsid, environ

class DependencyInstaller(object):

  L_BIN = path.join("/usr", "local", "bin")
  APPROVED_BREW_VERSION = "1.5.9"
  MINIMUM_PYTHON3 = '3.6.4'

  def __init__(self, options):
    if options.verbose:
      print('Initializing DependencyInstaller')
    self.options = options

  def isInstalled(self, dep):
    """Check whether a package @dep is installed"""
    # TODO: This check will work for executables, but checking whether something exists
    # is not enough. We need to ensure that the installed version has the required capabilities.
    # OR we need to check that brew has just installed it.  Since some things are just libs.
    if not path.exists(path.join(self.L_BIN, dep)):
      notFound = subprocess.call(['which', dep], stdout=subprocess.PIPE)
      if notFound:
        return False
    return True

  def isPythonPackageInstalled(self, package, version):
    pip = 'pip' + str(version)
    # this prints a warning that can be disabled with '--format=columns',
    # but that param isn't supported by older versions of pip
    allPackages = subprocess.check_output([pip, 'list'])
    isInstalled = package in allPackages
    return isInstalled

  def testHomebrew(self):
    """Test if homebrew is installed.  Installing this directly now requires user input."""

    noBrew = subprocess.call(['which', 'brew'], stdout=subprocess.PIPE)
    if noBrew:
       print( "Homebrew is not installed.  Goto https://brew.sh ")
       return False
    return True

  def mustUpgrade(self, tool, min_version):
    """Compare installed version to make sure its at least at the minimum value."""

    home_regex = re.compile('.*(\d+\.\d+\.\d+\S*)\s')

    try:
      stdout_result = subprocess.check_output([tool, '--version'])
      if stdout_result == "":
        print("info: {0} not installed at this time.".format(tool))
        return False
    except:
      print("info: {0} not installed at this time.".format(tool))
      return False

    current_version = home_regex.match(stdout_result)

    print("info: {0} {1}".format(tool, current_version.group(1)))

    return LooseVersion(min_version) > LooseVersion(current_version.group(1))

  def minimumNeededVersion(self, tool, version):
    """if tool is using a version that is too old it needs to be updated."""

    if self.mustUpgrade(tool, version):
      print("info: {0} is too old. Updating...".format(tool))
      if tool == 'brew':
         cmd = 'update'
      else:
         cmd = 'upgrade'
      result = subprocess.call(['brew', cmd])
      if result:
        print("error: failed to {0} {1}.".format(cmd, tool))
        return False
      return True
    return True

  def forceInstall(self, tool):
    """force installation of package.  this will be for packages that brew in its ignorance has deprecated."""
    print("force installing {0}".format(tool))
    result = subprocess.call(['brew', 'install', '--force', '--verbose', tool])
    if result:
      print("error: failed to install {0}!".format(tool))
      return False
    print("keg-only packages are not linked.".format(tool))
    print("warning: force linking {0}".format(tool))
    result = subprocess.call(['brew', 'link', '--force', '--verbose', tool])
    if result:
      print("error: failed to link {0}!".format(tool))
      return False
    if not self.isInstalled(tool):
      print("warning: {0} still not found at {1}".format(tool,self.L_BIN))
    return True


  def installTool(self, tool):
    if not self.isInstalled(tool):
      # todo: add support for installing specific versions.
      print "installing %s" % tool
      result = subprocess.call(['brew', 'install', tool])
      if result:
        print "error: failed to install %s!" % tool
        return False
      if not self.isInstalled(tool):
        print("warning: {0} still not found in {1}.  possibly keg-only".format(tool, self.L_BIN))
        return self.forceInstall(tool)
    return True

  def installPythonPackage(self, package, version):
    if not self.isPythonPackageInstalled(package, version):
      print "installing %s" % package
      pip = 'pip' + str(version)
      result = subprocess.call([pip, 'install', package])
      if result:
        print("error: failed to install python{0} package {1}!".format(version, package))
        return False
      if not self.isPythonPackageInstalled(package, version):
        print("error: python{0} package {1} still not installed!".format(version, package))
        return False
    return True


  def addEnvVariable(self, env_name, env_value):
    """ adds an env variable to bash profile & sets it for the run of the script.
    args:
        env_name: name of environment variable(will be caste to uppercase).
        env_value: value of variable.

    returns: True if set.  False is the variable exists either in the environment or in bash_profile.

    """
    env_name = str.upper(env_name)
    home = getenv("HOME")
    if home is None:
      print("error: unable to find ${HOME} directory")
      return False

    # the insurmountable problem is that a subprocess cannot change the environment of the calling process.
    # this will set everything for this run or for future terminals to work correctly.
    if getenv(env_name) is None:
      environ[env_name] = env_value
      print("Set environment variable {} to {}".format(env_name, env_value))

      bash_prof = path.join(home, '.bash_profile')
      export_line = 'export {}={}'.format(env_name, env_value)
      # if there is no bash profile don't' make it.
      if path.isfile(bash_prof):
        handle_prof = open(bash_prof, 'r')
        check_lines = handle_prof.readlines()
        handle_prof.close()
        for lines in check_lines:
          if lines.strip() == export_line.strip():
            #variable already set but doesn't exist in env. this implies that it's been added by this process.
            print("warning: you should open a new terminal now that bash_profile has been modified.")
            return False

        with open(bash_prof, 'a+') as file:
          file.write('\n' + export_line)
        if file.closed is True:
          # bash is not optional for this subprocess call.
          if subprocess.call(['source', bash_prof], executable="/bin/bash") != 0:
            print "warning: unable to source {}".format(bash_prof)

    return True

  def install(self):
    environ["PIP_REQUIRE_VIRTUALENV"] = ""
    homebrew_deps = self.options.deps

    python2_deps = self.options.python2_deps
    python3_deps = self.options.python3_deps

    if not self.testHomebrew():
      return False

    # dependency not existing wont break this.
    self.minimumNeededVersion('python3',self.MINIMUM_PYTHON3)
    self.minimumNeededVersion('brew',self.APPROVED_BREW_VERSION)

    for tool in homebrew_deps:
      if not self.installTool(tool):
        return False
    for package in python2_deps:
      if not self.installPythonPackage(package, 2):
        return False
    for package in python3_deps:
      if not self.installPythonPackage(package, 3):
        return False
    return True


def parseArgs(scriptArgs):
  version = '1.0'
  parser = argparse.ArgumentParser(description='runs homebrew to install required dependencies, and android package manager for android dependencies', version=version)
  parser.add_argument('--verbose', dest='verbose', action='store_true',
                      help='prints extra output')
  parser.add_argument('--dependencies', '-d', dest='deps', action='store', nargs='+',
                      help='list of dependencies to check and install')
  parser.add_argument('--pip2', dest='python2_deps', action='store', nargs='+',
                      help='list of python2 packages to check and install via pip2')
  parser.add_argument('--pip3', dest='python3_deps', action='store', nargs='+',
                      help='list of python3 packages to check and install via pip3')

  (options, args) = parser.parse_known_args(scriptArgs)
  return options



if __name__ == '__main__':
    # TODO: We should also specified required version where applicable
    # and figure out a way to only install the required versions.
    #deps = ['ninja', 'valgrind', 'android-ndk', 'android-sdk']
  options = parseArgs(sys.argv)
  installer = DependencyInstaller(options)
  if installer.install():
    sys.exit(0)
  else:
    sys.exit(1)

