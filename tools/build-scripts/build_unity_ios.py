#!/usr/bin/env python

import argparse
import fnmatch
import os
import re
import shutil
import sys

script_dir = os.path.dirname(os.path.abspath(os.path.realpath(__file__)))
sys.path.insert(0, os.path.join(script_dir, '..'))

import ankibuild.builder
import ankibuild.util
import ankibuild.unity

class UnityProjectIOS:
    """Wrapper for Unity project settings & info."""

    def __init__(self, project_dir, options=None):
        self.project_dir = os.path.abspath(project_dir)
        self.build_dir = os.path.join(ankibuild.util.Git.repo_root(), 'build')
        self.config = 'release'
        self.sdk = 'iphoneos'
        self.script_engine = 'mono'
        self.options = options

        if options is not None:
            self.build_dir = options.build_dir
            self.config = options.config
            self.sdk = options.sdk
            self.script_engine = options.script_engine

    def xcode_project_file(self):
        return 'Unity-iPhone.xcodeproj'

    def mono_generated_files(self):
        assemblies = [
            'Assembly-CSharp.dll.s',
            'Assembly-UnityScript.dll.s',
            'Mono.Security.dll.s',
            'System.Core.dll.s',
            'System.dll.s',
            'UnityEngine.UI.dll.s',
            'UnityEngine.dll.s',
            'mscorlib.dll.s',
            #    'RegisterMonoModules.h',
            'RegisterMonoModules.cpp',
            ]

        data = [
            'PlayerConnectionConfigFile',
            'mainData',
            '*.assets',
        ]

        data_managed = [
            'Assembly-CSharp.dll',
            'Assembly-CSharp.dll.mdb',
            'Assembly-UnityScript.dll',
            'Assembly-UnityScript.dll.mdb',
        ]

        libs = [os.path.join(build_dir(), 'Libraries', f) for f in assemblies]
        data = [os.path.join(build_dir(), 'Data', f) for f in data]
        managed = [os.path.join(build_dir(), 'Data', 'Managed', f) for f in data_managed]

        files = libs + data + managed

        return files

    def xcode_generated_file(self):
        return os.path.join(self.build_dir, 'Unity-iPhone.xcodeproj')

    def patch_register_mono_modules(self, input_io, output_io):
    
        stage = 'start'
        buf = []
        output = []

        for line in input_io:
            if stage == 'start':
                if re.search('^\#if \!\(TARGET_IPHONE_SIMULATOR\)', line):
                    stage = 'header'
                    continue
            elif stage == 'header':
                buf.append(line)
                if re.search('mono_dl_register_symbol', line):
                    output.extend(buf)
                    output.append("#if !(TARGET_IPHONE_SIMULATOR)\n")
                    del buf[:]
                    stage = 'scan'
                    continue
            elif stage == 'end':
                if re.search('^\#endif', line):
                    stage = 'done'
                    continue
            elif stage == 'footer':
                if re.search('mono_dl_register_symbol', line):
                    output.append("#endif\n")
                    output.append("\n")
                    stage = 'end'
            elif (stage == 'scan'):
                if re.search('void RegisterMonoModules', line):
                    stage = 'footer'

            if len(buf) == 0:
                output.append(line)

        output_io.write(''.join(output))

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
        for root, dirnames, filenames in os.walk(self.project_dir):
            for p in src_patterns:
                for filename in fnmatch.filter(filenames, p):
                    if filename in excludes:
                        continue
                    src_files.append(os.path.join(root, filename))

        return src_files

    def is_uptodate(self, src, dst):
        if not os.path.exists(src):
            return False
        if not os.path.exists(dst):
            return False

        src_time = os.path.getmtime(src)
        dst_time = os.path.getmtime(dst)

        return dst_time >= src_time

    def build_unity(self):
        unity_config = ankibuild.unity.UnityBuildConfig(self.project_dir)
        unity_config.set_options(self.options)

        unity_build = ankibuild.unity.UnityBuild(unity_config)
        ret = unity_build.run()

        # abort if unity build fails
        if ret != 0:
            return ret

        # patch RegisterMonoModules.cpp to work for Simulator builds
        cpp_file = os.path.join(self.build_dir, 'Libraries', 'RegisterMonoModules.cpp')
        cpp_file_in = "%s.in" % cpp_file

        is_patched = os.path.isfile(cpp_file_in) and self.is_uptodate(cpp_file_in, cpp_file)

        if not is_patched:
            # mv src cpp to cpp.in 
            shutil.move(cpp_file, cpp_file_in)

            io_in = open(cpp_file_in, 'r')
            io_out = open(cpp_file, 'w')
            cursor_in = io_out.tell()
            self.patch_register_mono_modules(io_in, io_out)
            cursor_out = io_out.tell()
            io_in.close()
            io_out.close()

            if cursor_out == cursor_in:
                ret = 1

            # check for patch failure
            if not os.path.isfile(cpp_file):
                ret = 1

        return ret

    def build(self):
        src_files = self.unity_source_files()
        project_file = self.xcode_generated_file()

#        up_to_date = True
#        for src in src_files:
#            if not self.is_uptodate(src, project_file):
#                up_to_date = False
#                break
#
#        if up_to_date:
#            print "unity up-to-date. skipping build."
#            return 0

        return self.build_unity() 


if __name__ == '__main__':
    args = sys.argv[1:]

    builder = ankibuild.builder.Builder()

    parser = argparse.ArgumentParser(parents=[builder.argument_parser()],
                                     description='Build a Unity iOS app')
    parser.add_argument('--sdk', action="store", choices=('iphoneos', 'iphonesimulator'), default="iphoneos")
    parser.add_argument('--script-engine', action="store", choices=('mono', 'il2cpp'), default="mono")
    parser.add_argument('--script-debugging', action="store_true", default=False,
                        help="Enable Mono script debugging in UnityEngine")
    parser.add_argument('--xcode-build-dir', action="store",
                        help="Path for Unity-generated Xcode project")
    parser.add_argument('unity_project', action="store", help="path to Unity project")

    parser.set_defaults(build_dir=os.path.join(ankibuild.util.Git.repo_root(), 'build', 'Unity'),
                        log_file=os.path.join(ankibuild.util.Git.repo_root(), 'build', 'Unity', 'UnityBuild-ios.log'),
                        platform='ios',
                        config='release')

    options = builder.parse_arguments(parser, args)

    if options.xcode_build_dir is None:
        unity_build_dir = os.path.abspath(os.path.join(options.build_dir, "%s-%s" % (options.config, options.sdk)))
    else:
        unity_build_dir = os.path.abspath(options.xcode_build_dir)

    options.build_dir = unity_build_dir
    options.build_product = os.path.join(options.build_dir, 'Unity-iPhone.xcodeproj')

    proj = UnityProjectIOS(options.unity_project, options)
    ret = proj.build()
    exit(ret)

