#!/usr/bin/env python

import sys
import subprocess
import argparse
import platform
import signal
from os import getenv, path, killpg, setsid, environ

class DependencyInstaller(object):

  OPT = path.join("/usr", "local", "opt")

  def __init__(self, options):
    if options.verbose:
      print('Initializing DependencyInstaller')
    self.options = options

  def isInstalled(self, dep):
    """Check whether a package @dep is installed"""
    # TODO: This check will work for executables, but checking whether something exists
    # is not enough. We need to ensure that the installed version has the required capabilities.
    if not path.exists(path.join("/usr/local/opt", dep)):
      notFound = subprocess.call(['which', dep], stdout=subprocess.PIPE)
      if notFound:
        return False
    return True


  def getHomebrew(self):
    """Install homebrew"""
    # TODO: Simply installing homebrew an using it is not enough, we need to handle:
    # - Updating homebrew if necessary
    # - Checking the versions of the packages that we need
    # - Making sure to install specific versions
    noBrew = subprocess.call(['which', 'brew'], stdout=subprocess.PIPE)
    if noBrew:
      if self.options.verbose:
        print "Homebrew not installed!"
      brewInstallFailed = subprocess.call(['ruby', '-e', "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"])
      if brewInstallFailed:
        print "Failed to install Homebrew!"
        return False
      noBrew = subprocess.call(['which', 'brew'], stdout=subprocess.PIPE)
      if noBrew:
        print "Homebrew still not found after install! (Check your path!)"
        return False
    return True


  def installTool(self, tool):
    if not self.isInstalled(tool):
      # TODO: Add support for installing specific versions.
      print "Installing %s" % tool
      result = subprocess.call(['brew', 'install', tool])
      if result:
        print "Error: Failed to install %s!" % tool
        return False
      if not self.isInstalled(tool):
        print "Error: %s still not installed!" % tool
        return False
    return True

  def addEnvVariable(self, env_name, env_value):
    """ Adds an Env variable to bash profile & sets it for the run of the script.
    Args:
        env_name: Name of environment variable(will be caste to uppercase).
        env_value: Value of variable.

    Returns: True if set.  False is the variable exists either in the environment or in bash_profile.

    """
    env_name = str.upper(env_name)
    home = getenv("HOME")
    if home is None:
      print("Error: Unable to find ${HOME} directory")
      return False

    # The insurmountable problem is that a subprocess cannot change the environment of the calling process.
    # This will set everything for this run OR for future terminals to work correctly.
    if getenv(env_name) is None:
      environ[env_name] = env_value
      print("Set environment variable {} to {}".format(env_name, env_value))

      bash_prof = path.join(home, '.bash_profile')
      export_line = 'export {}={}'.format(env_name, env_value)
      # If there is no bash profile don't' make it.
      if path.isfile(bash_prof):
        handle_prof = open(bash_prof, 'r')
        check_lines = handle_prof.readlines()
        handle_prof.close()
        for lines in check_lines:
          if lines.strip() == export_line.strip():
            #Variable already set but doesn't exist in env. This implies that it's been added by this process.
            print("Warning: You should open a new terminal now that bash_profile has been modified.")
            return False

        with open(bash_prof, 'a+') as file:
          file.write('\n' + export_line)
        if file.closed is True:
          # bash is not optional for this subprocess call.
          if subprocess.call(['source', bash_prof], executable="/bin/bash") != 0:
            print "Warning: Unable to source {}".format(bash_prof)

    return True

  def install(self):
    homebrew_deps = self.options.deps

    if not self.getHomebrew():
      return False

    for tool in homebrew_deps:
      if not self.installTool(tool):
        return False
    return True


def parseArgs(scriptArgs):
  version = '1.0'
  parser = argparse.ArgumentParser(description='runs homebrew to install required dependencies, and android package manager for android dependencies', version=version)
  parser.add_argument('--verbose', dest='verbose', action='store_true',
                      help='prints extra output')
  parser.add_argument('--dependencies', '-d', dest='deps', action='store', nargs='+',
                      help='list of dependencies to check and install')

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

