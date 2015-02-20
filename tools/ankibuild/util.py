import collections
import errno
import os
import re
import shutil
import sys
try:
    import subprocess32 as subprocess
except ImportError:
    import subprocess

class Git(object):

    @staticmethod
    def repo_root():
        p = subprocess.Popen(['git', 'rev-parse', '--show-toplevel'], stdout=subprocess.PIPE, cwd=".")
        out, err = p.communicate()
        retcode = p.poll()
        if retcode != 0:
            env_repo_root = os.environ['ANKI_BUILD_REPO_ROOT']
            if env_repo_root != None:
                out = env_repo_root
            else:
                out = ''
        return out.strip()

    @staticmethod
    def find_submodule_path(pattern):
        p = subprocess.Popen(['git', 'submodule', 'status'], stdout=subprocess.PIPE, cwd=".")
        out, err = p.communicate()
        retcode = p.poll()
        if retcode != 0:
            return None

        submodule_path = None
        status_lines = out.strip().rsplit('\n')
        for line in status_lines:
            m = re.search('(.?)([0-9A-Fa-f]{40}) ([^\(]+) (.*)', line)
            # print("0 " + m.group(0))
            # print("1 " + m.group(1))
            # print("2 " + m.group(2))
            # print("3 " + m.group(3))
            # print("4 " + m.group(4))
            path = m.group(3)

            path_match = re.search(pattern, path)
            if path_match is not None:
                submodule_path = path
                break
            
        return submodule_path

ECHO_ALL = True

class File(object):

    @staticmethod
    def _is_file_up_to_date(target, dep):
        if not os.path.exists(dep):
            return False
        if not os.path.exists(target):
            return False

        target_time = os.path.getmtime(target)
        dep_time = os.path.getmtime(dep)

        return dep_time >= target_time

    @staticmethod
    def is_up_to_date(target, deps):
        if not isinstance(deps, collections.Iterable):
            deps = [deps]

        is_up_to_date = True
        for dep in deps:
            up_to_date = File._is_file_up_to_date(target, dep)
            if not up_to_date:
                break

        return is_up_to_date
    
    @staticmethod
    def pwd():
        "Returns the absolute path of the current working directory."
        if ECHO_ALL:
            print('pwd')
        try:
            return os.path.abspath(os.getcwd())
        except OSError as e:
            sys.exit('ERROR: Failed to get working directory: {0}'.format(e.strerror))

    @staticmethod
    def cd(path):
        "Changes the current working directory to the given path."
        path = os.path.abspath(path)
        if ECHO_ALL:
            print('cd ' + path)
        try:
            os.chdir(path)
        except OSError as e:
            sys.exit('ERROR: Failed to change to directory {0}: {1}'.format(path, e.strerror))


    @staticmethod
    def mkdir_p(path):
        "Recursively creates new directories up to and including the given directory, if needed."
        path = os.path.abspath(path)
        if ECHO_ALL:
            print(File._escape(['mkdir', '-p', path]))
        try:
            os.makedirs(path)
        except OSError as e:
            if not os.path.isdir(path):
                sys.exit('ERROR: Failed to recursively create directories to {0}: {1}'.format(path, e.strerror))


    @staticmethod
    def ln_s(source, link_name):
        "Makes a symbolic link, pointing link_name to source."
        source = os.path.abspath(source)
        link_name = os.path.abspath(link_name)
        if ECHO_ALL:
            print(File._escape(['ln', '-s', source, link_name]))
        try:
            os.symlink(source, link_name)
        except OSError as e:
            if not os.path.islink(link_name):
                sys.exit('ERROR: Failed to create symlink from {0} pointing to {1}: {2}'.format(link_name, source, e.strerror))


    @staticmethod
    def rm_rf(path):
        "Removes all files and directories including the given path."
        path = os.path.abspath(path)
        if ECHO_ALL:
            print(File._escape(['rm', '-rf', path]))
        try:
            shutil.rmtree(path)
        except OSError as e:
            if os.path.exists(path):
                sys.exit('ERROR: Failed to recursively remove directory {0}: {1}'.format(path, e.strerror))


    @staticmethod
    def rm(path):
        "Removes a single specific file."
        path = os.path.abspath(path)
        if ECHO_ALL:
            print(File._escape(['rm', path]))
        try:
            os.remove(path)
        except OSError as e:
            if os.path.exists(path):
                sys.exit('ERROR: Failed to remove file {0}: {1}'.format(path, e.strerror))


    @staticmethod
    def rmdir(path):
        "Removes a directory if empty, or just contains directories."
        path = os.path.abspath(path)
        if ECHO_ALL:
            print(File._escape(['rmdir', path]))
        try:
            dirs = [path]
            cycle_detector = set()
            while dirs:
                dir = dirs.pop()
                
                # infinite symlink loop protection
                realpath = os.path.realpath(dir)
                if realpath in cycle_detector:
                    continue
                cycle_detector.add(realpath)
                
                for file in os.listdir(dir):
                    if file != '.DS_Store':
                        fullpath = os.path.join(dir, file)
                        if os.path.isdir(path):
                            dirs.append(path)
                        else:
                            # normal file, so don't remove
                            print('"{0}" still exists, so not removing "{1}"'.format(fullpath, path))
                            return
            
            shutil.rmtree(path)
        except OSError as e:
            if os.path.exists(path):
                sys.exit('ERROR: Failed to remove empty directory {0}: {1}'.format(path, e.strerror))


    @staticmethod
    def read(path):
        "Reads the contents of the file at the given path."
        path = os.path.abspath(path)
        if ECHO_ALL:
            print('reading all from {0}'.format(path))
        try:
            with open(path, 'r') as file:
                return file.read()
        except IOError as e:
            sys.exit('ERROR: Could not read from file {0}: {1}'.format(path, e.strerror))

    @staticmethod
    def write(path, contents):
        "Writes the specified contents to the given path."
        path = os.path.abspath(path)
        contents = str(contents)
        if ECHO_ALL:
            print('writing {0} bytes to {1}'.format(len(contents), path))
        try:
            with open(path, 'w') as file:
                file.write(contents)
        except IOError as e:
            sys.exit('ERROR: Could not write to file {0}: {1}'.format(path, e.strerror))


    @staticmethod
    def _escape(args):
        try:
            import pipes
            return ' '.join([pipes.quote(arg) for arg in args])
        except ImportError:
            return ' '.join(args)


    @staticmethod
    def _run_subprocess(func, args):
        if not args:
            raise ValueError('No arguments to execute.')
        try:
            return func(args)
        except subprocess.CalledProcessError as e:
            sys.exit('ERROR: Subprocess `{0}` exited with code {1}.'.format(File._escape(args), e.returncode)) 
        except OSError as e:
            sys.exit('ERROR: Error in subprocess `{0}`: {1}'.format(File._escape(args), e.strerror))


    @staticmethod
    def _raw_execute(func, args):
        print('')
        if ECHO_ALL:
            print(File._escape(args))
        return File._run_subprocess(func, args)


    @staticmethod
    def execute(args):
        "Executes the given list of arguments as a shell command, returning nothing."
        File._raw_execute(subprocess.check_call, args)


    @staticmethod
    def evaluate(args):
        "Executes the given list of arguments as a shell command, returning stdout."
        return File._raw_execute(subprocess.check_output, args)


