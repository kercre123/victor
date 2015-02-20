#!/usr/bin/python

import argparse
import errno
import fnmatch
import os
import re
import socket
import subprocess
import sys

# ankibuild
import util

class UnityBuildConfig(object):
    def __init__(self, project_dir):
        self.project_dir = project_dir
        self.build_type = 'release'
        self.build_product = None
        self.platform = 'osx'
        self.build_dir = os.path.join(util.Git.repo_root(), 'build', 'Unity')
        self.sdk = 'iphoneos'
        self.log_file = os.path.join(self.build_dir, 'UnityBuild.log')
        self.force = False

    def set_options(self, options):
        options_dict = vars(options)
        for k,v in options_dict.iteritems():
            if hasattr(self, k):
                setattr(self, k, v)

        if not options_dict.has_key('log_file'):
            self.log_file = os.path.join(os.path.dirname(self.build_dir), 'UnityBuild.log')


    def parse_arguments(self, argv):
        HELP_USAGE = """Usage:
            %s <options>""" % __name__

        parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, epilog=HELP_USAGE)
        parser.add_argument('--buildType', nargs='?', dest='buildType', default=self.build_type, help="Specify 'release' or 'debug'.") 
        parser.add_argument('--buildPlatform', nargs='?', dest='buildPlatform', default=self.platform, help="android/ios/osx") 
        parser.add_argument('--buildPath', nargs='?', dest='buildPath', default=self.build_dir, help="Path to build folder") 
        parser.add_argument('--buildProduct', nargs='?', dest='buildProduct', default=self.build_product, help="Path to product of Unity build process") 
        parser.add_argument('--sdk', nargs='?', dest='buildSDK', default=self.sdk, help="iphoneos/iphonesimulator") 
        parser.add_argument('--logFile', nargs='?', dest='logFile', default=self.log_file, help='Log build output to file.') 
        parser.add_argument('--force', action='store_true', dest='forceBuild', default=self.force, help='Build regardless of dependency checks.') 
        script_options = parser.parse_args(argv)

        self.build_type = script_options.buildType
        self.build_product = script_options.buildProduct
        self.platform = script_options.buildPlatform
        self.build_dir = script_options.buildPath
        self.sdk = script_options.buildSDK
        self.log_file = script_options.logFile
        self.force = script_options.forceBuild


class UnityBuild(object):
    """Wrapper class for building through Unity
    """

    def __init__(self, build_config):
        self.build_config = build_config

        # platform-dependenct build_product
        #if not build_config.build_product:
        #    if build_config.platform == 'ios':
        #        build_config.build_product = os.path.join(build_config.build_dir, 'Unity-iPhone.xcodeproj')
            # handle mac/osx case
        #    elif build_config.platform == 'mac':
        #        build_config.build_product = os.path.join(self.build_dir, 'UnityPlayer.app')

        #assert(build_config.build_product is not None)

    def check_log_file_for_autobuild_not_found(self, logFile):
        ret = subprocess.call(['egrep', '-q', "executeMethod class 'AutoBuild' could not be found.", logFile], cwd=".")
        return ret == 0

    def unity_source_files(self):
        src_patterns = [
            '*.cs',
            '*.unity',
            '*.prefab',
            '*.asset',
            '*.shader',
            '*.csproj',
            '*.unityproj'
        ]

        # We need to skip the ProjectSettings.asset file, since it can be
        # modified during the build.
        excludes = set(['ProjectSettings.asset'])

        src_files = []
        for root, dirnames, filenames in os.walk(self.build_config.project_dir):
            for p in src_patterns:
                for filename in fnmatch.filter(filenames, p):
                    if filename in excludes:
                        continue
                    src_files.append(os.path.join(root, filename))

        return src_files

    def need_to_build(self):
        src_files = self.unity_source_files()
        build_product = self.build_config.build_product

        return util.File.is_up_to_date(build_product, src_files)

    def parse_log_file(self, logFile):
        lines = [line.strip() for line in open(logFile)]
        reerror = re.compile(r"(error)|(warn)", re.I)
        reCompileStart = re.compile(r"-----CompilerOutput:-stderr----------", re.I)
        reCompileEnd = re.compile(r"-----EndCompilerOutput---------------", re.I)
        hasCompileOut = filter(lambda x:reCompileStart.search(x), lines)
        inCompilerOutSection = False
        for line in lines:

            if reCompileEnd.search(line):
                inCompilerOutSection = False
                break

            if hasCompileOut:
                if inCompilerOutSection:
                    print line
            else:
                if reerror.search(line):
                    print line

            if reCompileStart.search(line):
                inCompilerOutSection = True

    def make_build_dir(self):
        build_root = os.path.dirname(self.build_config.build_dir)
        if not os.path.exists(build_root):
            os.makedirs(build_root)

    def call_into_unity(self):
        message = ""
        # we don't need this, as unity will build currently opened project
        #message += '-projectPath ' + projectPath
        # we don't need this, as there is no way to redirect log after unity start up
        #message += " -logFile " + options.logFile 
        message += " --platform " + self.build_config.platform
        message += " --config " + self.build_config.build_type
        message += " --sdk " + self.build_config.sdk
        message += " --build-path " + self.build_config.build_dir

        logFilePath = os.path.expanduser('~/Library/Logs/Unity/Editor.log')
        handleUnityLog = open(logFilePath, 'r')
        handleUnityLog.seek(0, os.SEEK_END)

        unityOpen = True
        buildCompleted = False
        try:
            server_address = ('localhost', 48888)
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(server_address)
            print 'sending build request to unity ', message
            sock.sendall(message)

            # now wait for response
            while 1:
                data = sock.recv(4096)
                if not data: break
                print "received data:", data
                buildCompleted = True
                # no need to exit, as server will close the connection when done
        except socket.error as (socket_errno, msg):
            if (socket_errno == errno.ECONNREFUSED):
                unityOpen = False
                print "unity not started, running command line"
            else:
                print "error " + str(socket_errno) + " message: " + msg
        except:
            print "Unexpected error:", sys.exc_info()[0]

        # now copy the log data
        util.File.mkdir_p(os.path.dirname(self.build_config.log_file))
        handleBuildLog = open(self.build_config.log_file, 'w')
        logData = handleUnityLog.read()
        handleBuildLog.write(logData)
        handleUnityLog.close()
        handleBuildLog.close()
        return (unityOpen, buildCompleted, self.build_config.log_file)
                
    def run_unity_command_line(self):
        exe="/Applications/Unity/Unity.app/Contents/MacOS/Unity"
        procArgs = [exe, "-batchmode", "-quit", "-nographics"]
        procArgs.extend(["-projectPath", self.build_config.project_dir])
        procArgs.extend(["-executeMethod", 'CommandLineBuild.Build'])
        procArgs.extend(["-logFile", self.build_config.log_file])
        procArgs.extend(["--platform", self.build_config.platform])
        procArgs.extend(["--config", self.build_config.build_type])
        procArgs.extend(["--sdk", self.build_config.sdk])
        procArgs.extend(["--build-path", self.build_config.build_dir])

        projectPath = self.build_config.project_dir

        print(' '.join(procArgs))
        util.File.mkdir_p(os.path.dirname(self.build_config.log_file))
        ret = subprocess.call(procArgs, cwd=projectPath)
        print "ret = " + str(ret)

        # It was observed (see ANDRIVE-128), that a clean build may fail
        # due to a bug in Unity.  The marker of this failure is the AutoBuild class
        # not being found.
        # If this occurs, run the build one more time as a workaround
        if ret != 0 and self.check_log_file_for_autobuild_not_found(self.build_config.log_file):
            ret = subprocess.call(procArgs, cwd=projectPath)

        return (ret, self.build_config.log_file)

    def run(self):
        result = 0

        if not (self.build_config.force or self.need_to_build()):
            return result

        self.make_build_dir()
        util.File.mkdir_p(os.path.dirname(self.build_config.log_file))

        # try to call into unity
        (unityOpen, buildCompleted, logFilePath) = self.call_into_unity()

        # if unity is nto open, run command line
        if (not unityOpen):
            (result, logFilePath) = self.run_unity_command_line()

        if result != 0:
            return result

        self.parse_log_file(logFilePath)

        # Make sure a Build directory was created, otherwise fail the build
        if result == 0:
            result = 0 if os.path.isdir(self.build_config.build_dir) else 2
            if result != 0:
                print "'" + self.build_config.build_dir + "' does not exist"

        return result

# def main():
if __name__ == '__main__':

    argv = sys.argv[1:]

    config = UnityBuildConfig(os.path.join(util.Git.repo_root(), 'unity', 'OverDrive'))
    config.parse_arguments(argv)

    # platform-dependenct build_product
    if not config.build_product:
        if config.platform == 'ios':
            config.build_product = os.path.join(config.build_dir, 'Unity-iPhone.xcodeproj')
        # handle mac/osx case
        elif config.platform == 'osx':
            config.build_product = os.path.join(config.build_dir, 'UnityPlayerOSX.app')

    if (config.platform == 'osx'):
        config.build_dir = os.path.join(config.build_dir, 'UnityPlayerOSX.app')

    builder = UnityBuild(config)
    ret = builder.run()

    if ret == 0:
        print "Unity compile completed correctly."
    else:
        print "UNITY COMPILE ERROR"

    exit(ret)
