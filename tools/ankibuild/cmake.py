
"cmake generation."

import os.path
import util
import sys

CMAKE_TOOLCHAIN_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'iOS.cmake')

def generate(build_path, source_path, platform, generator=None):
    arguments = ['cmake']
    
    if generator is not None:
        arguments += ['-G', generator]
    elif platform == 'mac':
        arguments += ['-G', 'Xcode']
    elif platform == 'ios':
        arguments += ['-G', 'Xcode']
    else:
        sys.exit('Must specify cmake --generator for platform "{0}"'.format(platform))
    
    if platform == 'ios':
        arguments += ['-DANKI_IOS_BUILD=1', '-DCMAKE_TOOLCHAIN_FILE=' + CMAKE_TOOLCHAIN_FILE]
    
    arguments += [os.path.abspath(source_path)]
    
    cwd = util.File.pwd()
    util.File.mkdir_p(build_path)
    util.File.cd(build_path)
    util.File.execute(arguments)
    util.File.cd(cwd)

