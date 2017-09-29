ANDROID_NDK_ROOT:=~/.anki/android/ndk-repository/android-ndk-r15b

ANDROID_ARM_TOOLS_ROOT:=../../../generated/android/tools/arm
ANDROID_ARM_TOOLS_BIN:=$(ANDROID_ARM_TOOLS_ROOT)/bin
# Compilation Tools

CROSS_COMPILE = $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-
CFLAGS = -c -fPIC -Wall -pie


CROSS_COMPILE = $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-

# CC := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-gcc
# CPP := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-g++
# AR := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-ar

# Compiler flags
# Specify all the flags below which you want to use for your compilation, For this simple example, we only need to specify the include directory path
#CFLAGS          :=  -pie -I. -fPIC

# link with core lib
LIBS    += -lcore
LIBPATH += -L../../core
CFLAGS  += -I../../core/inc


# All the c files in this directory will be compiled
SRCDIR = .
# .o and other temp files go here
OBJDIR = .output


DFU = dfu
DFU_SRC = dfu.c
DFU_OBJ = $(DFU_SRC:%.c=$(OBJDIR)/%.o)




REMOTE_DEST = /data/local/fixture/

all: $(ANDROID_ARM_TOOLS_ROOT) $(DFU)

$(ANDROID_ARM_TOOLS_ROOT):
	$(ANDROID_NDK_ROOT)/build/tools/make_standalone_toolchain.py --arch arm --install-dir $(ANDROID_ARM_TOOLS_ROOT) --api 24

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CROSS_COMPILE)gcc $(CFLAGS) $<  -o $@


$(DFU): $(DFU_OBJ)
	$(CROSS_COMPILE)gcc -pie $(DFU_OBJ) $(LIBPATH) $(LIBS)  -o $(DFU)

push:
	adb push $(DFU)  $(REMOTE_DEST) 

execute:
	adb shell -x "cd $(REMOTE_DEST) && ./$(DFU) $(safe)"

clean:
	rm -f $(APP) $(OBJDIR)/*.o 
