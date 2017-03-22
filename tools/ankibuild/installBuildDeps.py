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


  def installAndroidSDK(self):
    if self.isInstalled('android'):
      return True
    if not self.installTool('android'):
      return False
    # this list should be passed in via configure
    ANDROID_PKGS= ['tools',
                   'platform-tools',
                   'build-tools-19.1.0',
                   'android-21',
                   'android-19',
                   'android-18',
                   'extra-android-m2repository',
                   'extra-android-support',
                   'build-tools-21.1.2',
                   'build-tools-21.1.1',
                   'build-tools-20.0.0',
                   'build-tools-19.0.3',
                   'extra-google-m2repository',
                   'extra-google-google_play_services']
    click_yes = "(sleep 5 && while [ 1 ]; do sleep 1; echo y ; done)"
    shellCommand = "android --silent update sdk --all --no-ui --filter %s" % ','.join(ANDROID_PKGS)
    p = subprocess.Popen([click_yes], stdout=subprocess.PIPE, shell=True, preexec_fn=setsid, close_fds=True)
    result = subprocess.call(shellCommand, executable="/bin/bash", stdin=p.stdout, stderr=subprocess.STDOUT, shell=True)
    # This is critical otherwise click_yes will happen forever.
    killpg(p.pid, signal.SIGUSR1)
    return result == 0

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


  def install(self):
    homebrew_deps = self.options.deps
    if 'android-sdk' in homebrew_deps:
      homebrew_deps.remove('android-sdk')
      sdk_installed = self.installAndroidSDK()
      if not sdk_installed:
        return False

    if not self.getHomebrew():
      return False

    if 'buck' in homebrew_deps:
      subprocess.call(['brew', 'tap', 'facebook/fb'])

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

