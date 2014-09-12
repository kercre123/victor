from distutils.core import setup
from Cython.Build import cythonize
from distutils.extension import Extension

import numpy

extensions = [
    Extension("anki", 
        ["anki.pyx",
        "anki_interface.cpp", 
        "../robot/src/array2d.cpp",
        "../robot/src/benchmarking.cpp",
        "../robot/src/comparisons.cpp",
        "../robot/src/compress.cpp",
        "../robot/src/decisionTree.cpp",
        "../robot/src/draw.cpp",
        "../robot/src/errorHandling.cpp",
        "../robot/src/find.cpp",
        "../robot/src/fixedLengthList.cpp",
        "../robot/src/flags.cpp",
        "../robot/src/geometry.cpp",
        "../robot/src/hostIntrinsics_m4.cpp",
        "../robot/src/interpolate.cpp",
        "../robot/src/matlabConverters.cpp",
        "../robot/src/matlabInterface.cpp",
        "../robot/src/matrix.cpp",
        "../robot/src/memory.cpp",
        "../robot/src/opencvLight.cpp",
        "../robot/src/sequences.cpp",
        "../robot/src/serialize.cpp",
        "../robot/src/trig_fast.cpp",
        "../robot/src/utilities.cpp",
        "../robot/src/utilities_c.c",
        "../shared/src/utilities_shared.cpp"         
         ],
        include_dirs = ["../include", 
                        "../../vision/include", 
                        numpy.get_include(),
                        "../../../../coretech-external/opencv-2.4.8/modules/core/include/",
                        "../../../../coretech-external/opencv-2.4.8/modules/highgui/include/",
                        "../../../../coretech-external/opencv-2.4.8/modules/imgproc/include/",
                        "../../../../coretech-external/opencv-2.4.8/modules/objdetect/include/"],
        libraries = ["z"],
        extra_compile_args = ["-DANKICORETECH_EMBEDDED_USE_MALLOC", "-DANKICORETECH_EMBEDDED_USE_ZLIB"]),
]

setup(
  name = "anki",
  ext_modules = cythonize(extensions),
)