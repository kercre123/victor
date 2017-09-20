ANDROID_NDK_ROOT:=~/.anki/android/ndk-repository/android-ndk-r15b

ANDROID_ARM_TOOLS_ROOT:=../../../generated/android/tools/arm
ANDROID_ARM_TOOLS_BIN:=$(ANDROID_ARM_TOOLS_ROOT)/bin
# Compilation Tools
CC := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-gcc
CPP := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-g++
AR := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-ar
# Compiler flags
# Specify all the flags below which you want to use for your compilation, For this simple example, we only need to specify the include directory path
CFLAGS          := -I.
CFLAGS          += -I../../generated/clad/robot
CFLAGS          += -I../../syscon
CFLAGS          += -DPLATFORM_ANDROID=1


DEPS = ../spine/spine_hal.h ../spine/spine_crc.h 
SRCS = ../spine/spine_hal.c ../spine/spine_crc.c dfu.c
OBJ = ../spine/spine_hal.o ../spine/spine_crc.o dfu.o

APP = dfu

all: $(ANDROID_ARM_TOOLS_ROOT) $(APP)

$(ANDROID_ARM_TOOLS_ROOT):
	$(ANDROID_NDK_ROOT)/build/tools/make_standalone_toolchain.py --arch arm --install-dir $(ANDROID_ARM_TOOLS_ROOT) --api 24

$(APP):  $(OBJ)
	$(CC) -pie -llog $(OBJ) -o $(APP)

clean:
	rm -f $(APP) *.o ../spine/*.o
