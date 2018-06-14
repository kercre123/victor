VICOS_BUILD_TOOL_VER:=0.9-r03

VICOS_ARM_TOOLS_ROOT:=~/.anki/vicos-sdk/dist/$(VICOS_BUILD_TOOL_VER)

VICOS_ARM_TOOLS_BIN:=$(VICOS_ARM_TOOLS_ROOT)/prebuilt/bin

# Compilation Tools

CROSS_COMPILE = $(VICOS_ARM_TOOLS_BIN)/arm-oe-linux-gnueabi-

ARMCC = $(CROSS_COMPILE)clang

CFLAGS = -c -fPIC -Wall -Werror

CORE_DIR = ../../core
CORE = $(CORE_DIR)/libcore.a

# Compiler flags
# Specify all the flags below which you want to use for your compilation, For this simple example, we only need to specify the include directory path
#CFLAGS          :=  -pie -I. -fPIC

# link with core lib
LIBS    += -lcore
LIBPATH += -L$(CORE_DIR)
CFLAGS  += -I../ -O2
CFLAGS  += -I$(CORE_DIR)/inc

CFLAGS  += -I../../syscon
CFLAGS  += -I../../hal


# All the c files in this directory will be compiled
SRCDIR = .
# .o and other temp files go here
OBJDIR = .output


# CPPUTEST_HOME = /usr/local/Cellar/cpputest/3.8

# DEFINES += -DCOZMO_ROBOT

# CPPFLAGS += -I$(CPPUTEST_HOME)
# CPPFLAGS += -I"../../robot/include"
# CPPFLAGS += -I"../hal/include"
# # CPPFLAGS += -I"../../robot/generated/clad/robot"
# CPPFLAGS += $(DEFINES)

# CXXFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorNewMacros.h
# CFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorMallocMacros.h

# LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt
# LDFLAGS += -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt -lc++

# CODE_UNDER_TEST = ../hal/src/hal.cpp

# ifeq ($(PLATFORM),OSX)
# 	DEFINES += -DPLATFORM_OSX
#   CC = gcc
# else
# 	DEFINES += -DPLATFORM_ANDROID
#   CC = ~/arm/bin/clang -pie
# 	LOCAL_LDLIBS += -llog
# endif


lcd_test:  lcd_test.c
	$(ARMCC) $(CFLAGS) lcd_test.c -o lcd_test



app:
	gcc $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(CODE_UNDER_TEST)

test: app
	./a.out -c

$(OBJDIR)/spine_hal.o:
	$(CROSS_COMPILE)clang $(CFLAGS) ../../hal/spine/spine_hal.c  -o $@
$(OBJDIR)/spine_crc.o:
	$(CROSS_COMPILE)clang $(CFLAGS) ../../hal/spine/spine_crc.c  -o $@



%.o: %.c $(DEPS)
	$(CC)  -I../spine -c -o $@ $< $(CFLAGS)

spine_loop: ../hal/spine/spine.c spine_loop.c
	echo $(PLATFORMj)
	$(ARMCC) $(DEFINES) $(LOCAL_LDLIBS) -llog -g -O2 -I../ -I../hal/spine -I../syscon ../hal/spine/spine.c ../hal/spine/spine_crc.c -o spine_loop spine_loop.c
	adb push ./spine_loop /data/local/tmp


stupid:
	echo $(PLATFORMj)
#	$(CC) -v $(DEFINES) $(LOCAL_LDLIBS) -gcc-toolchain -L../spine -lspine stupid_hal.c
	$(ARMCC) $(DEFINES) $(LOCAL_LDLIBS) -I../hal/spine -I../syscon -llog ../hal/spine/spine_hal.c ../hal/spine/spine_crc.c stupid_hal.c
	adb push ./a.out /data/local/tmp

spinal_tap:
	$(CC) $(DEFINES) $(LOCAL_LDLIBS) -I../hal/spine -I../syscon  ../hal/spine/spine_hal.c ../hal/spine/spine_crc.c spinal_tap.c -o spinal_tap

imu_test: imu_test.c ../hal/spine/imu.c
	$(ARMCC) $(DEFINES) $(LOCAL_LDLIBS) -llog -I../hal/include -I../hal/spine ../hal/spine/imu.c imu_test.c -o imu_test

clean:
	rm -f *.o imu_test spinal_tap stupid_hal a.out lcd_test
