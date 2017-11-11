#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Downloads simpleperf prebuilts from the build server."""
import argparse
import glob
import logging
import os
import shutil
import stat
import textwrap


THIS_DIR = os.path.realpath(os.path.dirname(__file__))


class InstallEntry(object):
    def __init__(self, target, name, install_path, need_strip=False):
        self.target = target
        self.name = name
        self.install_path = install_path
        self.need_strip = need_strip


bin_install_list = [
    # simpleperf on device
    InstallEntry('sdk_arm64-sdk', 'simpleperf', 'android/arm64/simpleperf'),
    InstallEntry('sdk_arm64-sdk', 'simpleperf32', 'android/arm/simpleperf'),
    InstallEntry('sdk_x86_64-sdk', 'simpleperf', 'android/x86_64/simpleperf'),
    InstallEntry('sdk_x86_64-sdk', 'simpleperf32', 'android/x86/simpleperf'),

    # simpleperf on host
    InstallEntry('sdk_arm64-sdk', 'simpleperf_host', 'linux/x86_64/simpleperf', True),
    InstallEntry('sdk_arm64-sdk', 'simpleperf_host32', 'linux/x86/simpleperf', True),
    InstallEntry('sdk_mac', 'simpleperf_host', 'darwin/x86_64/simpleperf'),
    InstallEntry('sdk_mac', 'simpleperf_host32', 'darwin/x86/simpleperf'),
    # simpleperf.exe on x86_64 windows doesn't work, use simpleperf32.exe instead.
    InstallEntry('sdk', 'simpleperf32.exe', 'windows/x86_64/simpleperf.exe', True),
    InstallEntry('sdk', 'simpleperf32.exe', 'windows/x86/simpleperf.exe', True),

    # libsimpleperf_report.so on host
    InstallEntry('sdk_arm64-sdk', 'libsimpleperf_report.so', 'linux/x86_64/libsimpleperf_report.so', True),
    InstallEntry('sdk_arm64-sdk', 'libsimpleperf_report32.so', 'linux/x86/libsimpleperf_report.so', True),
    InstallEntry('sdk_mac', 'libsimpleperf_report.dylib', 'darwin/x86_64/libsimpleperf_report.dylib'),
    InstallEntry('sdk_mac', 'libsimpleperf_report32.so', 'darwin/x86/libsimpleperf_report.dylib'),
    InstallEntry('sdk', 'libsimpleperf_report.dll', 'windows/x86_64/libsimpleperf_report.dll', True),
    InstallEntry('sdk', 'libsimpleperf_report32.dll', 'windows/x86/libsimpleperf_report.dll', True),

    # libwinpthread-1.dll on windows host
    InstallEntry('local:../../prebuilts/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/x86_64-w64-mingw32/bin/libwinpthread-1.dll',
                 'libwinpthread-1.dll', 'windows/x86_64/libwinpthread-1.dll', False),
    InstallEntry('local:../../prebuilts/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/x86_64-w64-mingw32/lib32/libwinpthread-1.dll',
                 'libwinpthread-1_32.dll', 'windows/x86/libwinpthread-1.dll', False)
]

script_install_entry = InstallEntry('sdk_arm64-sdk', 'simpleperf_script.zip', 'simpleperf_script.zip')


def logger():
    """Returns the main logger for this module."""
    return logging.getLogger(__name__)


def check_call(cmd):
    """Proxy for subprocess.check_call with logging."""
    import subprocess
    logger().debug('check_call `%s`', ' '.join(cmd))
    subprocess.check_call(cmd)


def fetch_artifact(branch, build, target, pattern):
    """Fetches and artifact from the build server."""
    logger().info('Fetching %s from %s %s (artifacts matching %s)', build,
                  target, branch, pattern)
    if target.startswith('local:'):
        shutil.copyfile(target[6:], pattern)
        return
    fetch_artifact_path = '/google/data/ro/projects/android/fetch_artifact'
    cmd = [fetch_artifact_path, '--branch', branch, '--target', target,
           '--bid', build, pattern]
    check_call(cmd)


def start_branch(build):
    """Creates a new branch in the project."""
    branch_name = 'update-' + (build or 'latest')
    logger().info('Creating branch %s', branch_name)
    check_call(['repo', 'start', branch_name, '.'])


def commit(branch, build, add_paths):
    """Commits the new prebuilts."""
    logger().info('Making commit')
    check_call(['git', 'add'] + add_paths)
    message = textwrap.dedent("""\
        Update NDK prebuilts to build {build}.

        Taken from branch {branch}.""").format(branch=branch, build=build)
    check_call(['git', 'commit', '-m', message])


def list_prebuilts():
    """List all prebuilts in current directory."""
    result = []
    for d in ['bin', 'doc', 'inferno', 'testdata']:
        if os.path.isdir(d):
            result.append(d)
    result += glob.glob('*.py') + glob.glob('*.js')
    result.remove('update.py')
    result += ['inferno.sh', 'inferno.bat']
    return result


def remove_old_release():
    """Removes the old prebuilts."""
    old_prebuilts = list_prebuilts()
    if not old_prebuilts:
        return
    logger().info('Removing old prebuilts %s', old_prebuilts)
    check_call(['git', 'rm', '-rf', '--ignore-unmatch'] + old_prebuilts)

    # Need to check again because git won't remove directories if they have
    # non-git files in them.
    check_call(['rm', '-rf'] + old_prebuilts)


def install_new_release(branch, build):
    """Installs the new release."""
    for entry in bin_install_list:
        install_entry(branch, build, 'bin', entry)
    install_entry(branch, build, '.', script_install_entry)
    install_repo_prop(branch, build)


def install_entry(branch, build, install_dir, entry):
    """Installs one prebuilt file specified by entry."""
    target = entry.target
    name = entry.name
    install_path = os.path.join(install_dir, entry.install_path)
    need_strip = entry.need_strip

    fetch_artifact(branch, build, target, name)
    exe_stat = os.stat(name)
    os.chmod(name, exe_stat.st_mode | stat.S_IEXEC)
    if need_strip:
        check_call(['strip', name])
    dir = os.path.dirname(install_path)
    if not os.path.isdir(dir):
        os.makedirs(dir)
    shutil.move(name, install_path)

    if install_path.endswith('.zip'):
        unzip_simpleperf_scripts(install_path)


def unzip_simpleperf_scripts(zip_path):
    check_call(['unzip', zip_path])
    os.remove(zip_path)
    check_call(['mv'] + glob.glob('scripts/*') + ['.'])
    shutil.rmtree('scripts')
    check_call(['mv'] + glob.glob('demo/*') + ['testdata'])
    shutil.rmtree('demo')
    check_call(['mv'] + glob.glob('script_testdata/*') + ['testdata'])
    shutil.rmtree('script_testdata')


def install_repo_prop(branch, build):
    """Installs the repo.prop from the build for auditing."""
    # We took everything from the same build number, so we only need the
    # repo.prop from one of our targets.
    fetch_artifact(branch, build, 'sdk', 'repo.prop')


def get_args():
    """Parses and returns command line arguments."""
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-b', '--branch', default='aosp-master',
        help='Branch to pull build from.')
    parser.add_argument('--build', required=True, help='Build number to pull.')
    parser.add_argument(
        '--use-current-branch', action='store_true',
        help='Perform the update in the current branch. Do not repo start.')
    parser.add_argument(
        '-v', '--verbose', action='count', default=0,
        help='Increase output verbosity.')

    return parser.parse_args()


def main():
    """Program entry point."""
    os.chdir(THIS_DIR)

    args = get_args()
    verbose_map = (logging.WARNING, logging.INFO, logging.DEBUG)
    verbosity = args.verbose
    if verbosity > 2:
        verbosity = 2
    logging.basicConfig(level=verbose_map[verbosity])

    if not args.use_current_branch:
        start_branch(args.build)
    remove_old_release()
    install_new_release(args.branch, args.build)
    artifacts = ['repo.prop'] + list_prebuilts()
    commit(args.branch, args.build, artifacts)


if __name__ == '__main__':
    main()
