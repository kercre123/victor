ANDROID_NDK_BIN:=~/arm/bin
# Compilation Tools
CC := $(ANDROID_NDK_BIN)/arm-linux-androideabi-gcc
CPP := $(ANDROID_NDK_BIN)/arm-linux-androideabi-g++
AR := $(ANDROID_NDK_BIN)/arm-linux-androideabi-ar
# Compiler flags
# Specify all the flags below which you want to use for your compilation, For this simple example, we only need to specify the include directory path
CFLAGS          := -I.
CFLAGS          += -I../../generated/clad/robot
CFLAGS          += -DPLATFORM_ANDROID=1

# Rule for making C++ object file
.cpp.o:
	$(CPP) $(CFLAGS) -c  $<
# Rule for making C object file

.c.o: $(OBJ)
	$(CC) $(CFLAGS) -c  $<

DEPS = ../spine/spine_hal.h ../spine/spine_crc.h
SRCS = ../spine/spine_hal.c ../spine/spine_crc.c dfu.c 
OBJ = ../spine/spine_hal.o ../spine/spine_crc.o dfu.o
OBJL = spine_hal.o spine_crc.o dfu.o

all: dfu

faker: $(OBJ)
	touch faker

APP = dfu
$(APP): faker $(OBJL) 
	$(CC) -pie -llog $(OBJL) -o $(APP)



