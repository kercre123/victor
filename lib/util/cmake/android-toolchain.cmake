set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION 21) # API level
set(CMAKE_ANDROID_ARCH_ABI armeabi-v7a)
#set(ANDROID_ABI "armeabi-v7a with NEON")
set(ANDROID_ABI armeabi-v7a)
set(CMAKE_ANDROID_NDK $ENV{HOME}/.anki/android/ndk-repository/android-ndk-r14b)
set(ANDROID_NDK $ENV{HOME}/.anki/android/ndk-repository/android-ndk-r14b)
set(CMAKE_ANDROID_STL_TYPE c++_shared)
set(ANDROID_TOOLCHAIN clang)
#set(ANDROID_ARM_NEON TRUE)
set(ANDROID_TOOLCHAIN_NAME=arm-linux-androideabi-clang3.5)



set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
