
import os
import glob
from distutils.core import setup
from distutils.extension import Extension


# Change this to use 'boost_python3-mt' for Python 3
LIBRARIES = ['boost_python-mt',
             # the following OpenCV libs are from EXTERNALS/coretech_external/build/opencv-3.1.0/lib/Release/
             'opencv_core', 'opencv_imgproc', 'opencv_highgui', 'opencv_imgcodecs', 'opencv_calib3d',
             # the following third-party libs are from EXTERNALS/coretech_external/build/opencv-3.1.0/3rdparty/lib/Release/
             'libjasper', 'libpng', 'libtiff', 'IlmImf',
             'turbojpeg']

CTE_DIR = os.path.join('EXTERNALS', 'coretech_external')

OPENCV_BUILD_DIR = os.path.join(CTE_DIR, 'build', 'opencv-3.1.0')
OPENCV_LIBS_DIR = os.path.join(OPENCV_BUILD_DIR, 'lib', 'Release')
OPENCV_THIRD_PARTY_LIBS_DIR = os.path.join(OPENCV_BUILD_DIR, '3rdparty', 'lib', 'Release')
OPENCV_MODULES_DIR = os.path.join(CTE_DIR, 'opencv-3.1.0', 'modules')
OPENCV_INCLUDE_DIRS = glob.glob(os.path.join(OPENCV_MODULES_DIR, '*', 'include'))

JPEG_LIBS_DIR = os.path.join(CTE_DIR, 'libjpeg-turbo', 'mac_libs')

FLATBUFFERS_INCLUDE_DIR = os.path.join(CTE_DIR, 'flatbuffers', 'include')

PYTHON_BINDINGS_SOURCE_FILES = [os.path.join('python', 'cozmo_basestation_py.cpp')]

BASESTATION_DIR = os.path.join('basestation', 'src', 'anki', 'cozmo', 'basestation')
BASESTATION_SOURCE_FILES = {  None: ['proceduralFace.cpp', 'proceduralFaceDrawer.cpp', 'scanlineDistorter.cpp'] }

UTIL_DIR = os.path.join('lib', 'util', 'source', 'anki', 'util')
UTIL_SOURCE_FILES = { 'logging':   ['logging.cpp', 'channelFilter.cpp', 'callstack.cpp', 'iLoggerProvider.cpp'],
                      'console':   ['consoleVariable.cpp', 'consoleSystem.cpp', 'consoleSystemAPI.cpp',
                                    'consoleFunction.cpp', 'consoleArgument.cpp'],
                      'helpers':   ['cConversionHelpers.cpp'],
                      'global':    ['globalDefinitions.cpp'],
                      'fileUtils': ['fileUtils.cpp'],
                      'random':    ['randomGenerator.cpp'],
                      'string':    ['stringHelpers.cpp'],
                      'stringTable': ['stringTable.cpp', 'stringID.cpp'] }

JSON_SOURCE_FILES = [os.path.join('lib', 'util', 'source', '3rd', 'jsoncpp', 'jsoncpp.cpp')]

GENERATED_CLAD_DIR = os.path.join('generated', 'clad', 'engine', 'clad')
GENERATED_CLAD_SOURCE_FILES = { 'types': ['proceduralEyeParameters.cpp'] }

CORETECH_DIR = 'coretech'
CORETECH_SOURCE_FILES = { os.path.join('vision', 'basestation', 'src'): ['image.cpp'],
                          os.path.join('common', 'shared', 'src'): ['radians.cpp'],
                          os.path.join('common', 'basestation', 'src'): ['jsonTools.cpp', 'colorRGBA.cpp',
                                                                         os.path.join('math', 'rotation.cpp'),
                                                                         os.path.join('math', 'point.cpp')] }

INCLUDE_DIRS = [ os.path.join('basestation', 'include'),
                 os.path.join('include', 'anki', 'cozmo'),
                 os.path.join('tools', 'message-buffers', 'support', 'cpp', 'include'),
                 os.path.join('lib', 'util', 'source', 'anki'),
                 os.path.join('lib', 'util', 'source', '3rd'),
                 os.path.join('coretech', 'common', 'include'),
                 os.path.join('coretech', 'vision', 'include'),
                 os.path.join('coretech', 'generated', 'clad', 'common'),
                 os.path.join('coretech', 'generated', 'clad', 'vision'),
                 os.path.join('generated', 'clad', 'engine') ]
INCLUDE_DIRS.extend(OPENCV_INCLUDE_DIRS)
INCLUDE_DIRS.append(FLATBUFFERS_INCLUDE_DIR)

os.environ["CC"] = "clang++"
os.environ["CXX"] = "clang++"


def get_source_files():
    source_files = []
    for subdir, files in CORETECH_SOURCE_FILES.items():
        for source_file in files:
            source_files.append(os.path.join(CORETECH_DIR, subdir, source_file))
    for subdir, files in UTIL_SOURCE_FILES.items():
        for source_file in files:
            source_files.append(os.path.join(UTIL_DIR, subdir, source_file))
    for subdir, files in GENERATED_CLAD_SOURCE_FILES.items():
        for source_file in files:
            source_files.append(os.path.join(GENERATED_CLAD_DIR, subdir, source_file))
    for subdir, files in BASESTATION_SOURCE_FILES.items():
        for source_file in files:
            if subdir:
                source_files.append(os.path.join(BASESTATION_DIR, subdir, source_file))
            else:
                source_files.append(os.path.join(BASESTATION_DIR, source_file))
    source_files.extend(JSON_SOURCE_FILES)
    source_files.extend(PYTHON_BINDINGS_SOURCE_FILES)
    return source_files


cozmo_basestation_py = Extension('cozmo_basestation_py',
    sources = get_source_files(),
    include_dirs = INCLUDE_DIRS,
    library_dirs = [OPENCV_LIBS_DIR, OPENCV_THIRD_PARTY_LIBS_DIR, JPEG_LIBS_DIR],
    define_macros = [('ANKICORETECH_USE_OPENCV', '1')],
    libraries = LIBRARIES,
    extra_compile_args = ['-std=c++11','-stdlib=libc++', '-arch', 'x86_64'],
    extra_link_args = ['-stdlib=libc++', '-arch', 'x86_64']
    )


setup(name='cozmo_basestation', version='0.1', ext_modules=[cozmo_basestation_py])


