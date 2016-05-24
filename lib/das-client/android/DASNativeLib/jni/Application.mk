# select target arch
APP_ABI := armeabi-v7a

# impacts linking of breakpad
APP_PROJECT_PATH := $(shell pwd)

# CLANG versions available
NDK_TOOLCHAIN_VERSION := clang3.5

# target android platform
APP_PLATFORM := android-18

# which STL lib to use..
APP_STL := c++_shared

# enable c++11 extentions in source code
# using c++_std will enable this by default
# clang and gcc 4.8
APP_CPPFLAGS += -std=gnu++11
#APP_CPPFLAGS += -std=c++11
# gcc 4.6
#APP_CPPFLAGS += -std=c++0x

# enable c11
APP_CFLAGS += -std=c11

