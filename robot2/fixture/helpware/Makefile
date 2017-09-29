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
CFLAGS          += -I../../syscon
CFLAGS          += -DPLATFORM_ANDROID=1


DEPS = 
SRCS = dfu.c
OBJ = dfu.o

APP = dfu

DEST = /data/local/fixture/

all: $(ANDROID_ARM_TOOLS_ROOT) $(APP) push

$(ANDROID_ARM_TOOLS_ROOT):
	$(ANDROID_NDK_ROOT)/build/tools/make_standalone_toolchain.py --arch arm --install-dir $(ANDROID_ARM_TOOLS_ROOT) --api 24

$(APP):  $(OBJ)
	$(CC) -pie -llog $(OBJ) -o $(APP)

push:
	adb push $(APP)  $(DEST) 

clean:
	rm -f $(APP) *.o 
