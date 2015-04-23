# Compile by typing "python setup.py install"
# Requires python, with the numpy and cython packages installed

from distutils.core import setup
from Cython.Build import cythonize
from distutils.extension import Extension

import numpy

extensions = [
    Extension("anki",
        ["anki.pyx",
        "anki_interface.cpp",
        "../../coretech/common/robot/src/array2d.cpp",
        "../../coretech/common/robot/src/benchmarking.cpp",
        "../../coretech/common/robot/src/comparisons.cpp",
        "../../coretech/common/robot/src/compress.cpp",
        "../../coretech/common/robot/src/decisionTree.cpp",
        "../../coretech/common/robot/src/draw.cpp",
        "../../coretech/common/robot/src/errorHandling.cpp",
        "../../coretech/common/robot/src/find.cpp",
        "../../coretech/common/robot/src/fixedLengthList.cpp",
        "../../coretech/common/robot/src/flags.cpp",
        "../../coretech/common/robot/src/geometry.cpp",
        "../../coretech/common/robot/src/hostIntrinsics_m4.cpp",
        "../../coretech/common/robot/src/interpolate.cpp",
        "../../coretech/common/robot/src/matrix.cpp",
        "../../coretech/common/robot/src/memory.cpp",
        "../../coretech/common/robot/src/opencvLight.cpp",
        "../../coretech/common/robot/src/sequences.cpp",
        "../../coretech/common/robot/src/serialize.cpp",
        "../../coretech/common/robot/src/trig_fast.cpp",
        "../../coretech/common/robot/src/utilities.cpp",
        "../../coretech/common/robot/src/utilities_c.c",
        "../../coretech/common/shared/src/utilities_shared.cpp"
         ],
        include_dirs = ["../../include",
                        "../../coretech/common/include",
                        "../../coretech/vision/include",
                        numpy.get_include()],
        libraries = ["z"],
        extra_compile_args = ["-DANKICORETECH_EMBEDDED_USE_MALLOC",
                              "-DANKICORETECH_EMBEDDED_USE_ZLIB"]),
]

setup(
  name = "anki",
  ext_modules = cythonize(extensions),
)