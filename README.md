coretech-externals
==================

Non-Anki code we use or depend on in the CoreTech libraries.

VERY IMPORTANT: The acceptability of the license for any code added here *must* be ensured.

==================

To build gtest1.7.0 with Xcode on Mac:
1. Use Cmake as normal, with the build directory as coretech-external/build/xcode4/gtest-1.7.0
2. Open the Xcode project. For targets gtest and gtest_main, change
    a. C++ Language Dialect = c++11 [-std=c++11]
    b. C++ Standard Library = libc++ (LLVM C++ standard library with C++11 support)
3. Then build as normal

To build gtest1.7.0 with MSVC2012 or MSVC2010 on Windows:
1. Use Cmake as normal, with the build directory as coretech-external/build/msvc2012/gtest-1.7.0
2. Open the MSVC solution. For projects gtest and gtest_main:
	2a. Add "_VARIADIC_MAX=10" to Configuration Properties->C/C++->Preprocessor->Preprocessor Definitions:
	2b. Change Configuration Properties->C/C++->Code Generation->Runtime Library to "Multi-threaded DLL (/MD)" for Release, and "Multi-threaded Debug DLL (/MDd)" for Debug.
3. Then build as normal
4. For your projects that use gtest, you also must add "_VARIADIC_MAX=10" to the project properties

To build and use OpenCV 2.4.6.1 on Mac
1. Use Cmake as normal, with the build directory as coretech-external/build/xcode4/opencv-2.4.6.1

To build and use OpenCV 2.4.6.1 on Windows
1. Use Cmake as normal, with the build directory as coretech-external/build/msvc2012/opencv-2.4.6.1
2. Add the full directories for "[PATH]\coretech-external\build\msvc2012\opencv-2.4.6.1\bin\Debug" and "[PATH]\coretech-external\build\msvc2012\opencv-2.4.6.1\bin\Release" to your PATH variable (under Control Panel->All Control Panel Items->System->Advanced system settings->Advanced->Environment Variables->System variables).

