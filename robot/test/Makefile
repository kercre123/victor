
CPPUTEST_HOME = /usr/local/Cellar/cpputest/3.8

DEFINES += -DCOZMO_ROBOT

CPPFLAGS += -I$(CPPUTEST_HOME)
CPPFLAGS += -I"../../robot/include"
CPPFLAGS += -I"../hal/include"
# CPPFLAGS += -I"../../robot/generated/clad/robot"
CPPFLAGS += $(DEFINES)

CXXFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorNewMacros.h
CFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorMallocMacros.h

LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt
LDFLAGS += -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt -lc++

CODE_UNDER_TEST = ../hal/src/hal.cpp

ifeq ($(PLATFORM),OSX)
	DEFINES += -DPLATFORM_OSX
  CC = gcc
else
	DEFINES += -DPLATFORM_ANDROID
  CC = ~/arm/bin/clang -pie
	LOCAL_LDLIBS += -llog 
endif


#
# Use android.py to locate Android NDK
#
TOPLEVEL := ../..
ANDROID_PY := $(TOPLEVEL)/tools/build/tools/ankibuild/android.py
ANDROID_NDK_ROOT := $(shell $(ANDROID_PY))

# Android arm build toolchain
ANDROID_ARM_TOOLS_ROOT := $(TOPLEVEL)/generated/android/tools/arm
ANDROID_ARM_TOOLS_BIN := $(ANDROID_ARM_TOOLS_ROOT)/bin

# Compilation Tools
ARMCC := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-gcc -pie
ARMCPP := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-g++
ARMAR := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-ar



$(ANDROID_ARM_TOOLS_ROOT):
	$(ANDROID_NDK_ROOT)/build/tools/make_standalone_toolchain.py --arch arm --install-dir $(ANDROID_ARM_TOOLS_ROOT) --api 24

lcd_test:  $(ANDROID_ARM_TOOLS_ROOT) lcd_test.c
	$(ARMCC) -pie -llog lcd_test.c -o lcd_test



app: 
	gcc $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(CODE_UNDER_TEST)

test: app
	./a.out -c

%.o: %.c $(DEPS)
	$(CC)  -I../spine -c -o $@ $< $(CFLAGS)

spine_loop: ../hal/spine/spine.c spine_loop.c
	echo $(PLATFORMj)
	$(ARMCC) $(DEFINES) $(LOCAL_LDLIBS) -g -O2 -I../ -I../hal/spine -I../syscon ../hal/spine/spine.c ../hal/spine/spine_crc.c -o spine_loop spine_loop.c
	adb push ./spine_loop /data/local/tmp


stupid:
	echo $(PLATFORMj)
#	$(CC) -v $(DEFINES) $(LOCAL_LDLIBS) -gcc-toolchain -L../spine -lspine stupid_hal.c
	$(ARMCC) $(DEFINES) $(LOCAL_LDLIBS) -I../hal/spine -I../syscon ../hal/spine/spine_hal.c ../hal/spine/spine_crc.c stupid_hal.c 
	adb push ./a.out /data/local/tmp

bmi160:
	echo $(PLATFORM)
	$(ARMCC) $(DEFINES) $(LOCAL_LDLIBS) ../bmi160/bmi160.c ../bmi160/main.c



spinal_tap:
	$(CC) $(DEFINES) $(LOCAL_LDLIBS) -I../hal/spine -I../syscon  ../hal/spine/spine_hal.c ../hal/spine/spine_crc.c spinal_tap.c -o spinal_tap

imu_test: imu_test.c ../hal/spine/imu.c
	$(ARMCC) $(DEFINES) $(LOCAL_LDLIBS) -I../hal/include -I../hal/spine ../hal/spine/imu.c imu_test.c -o imu_test

imu_bmi160_test: imu_bmi160_test.c 
	echo $(PLATFORMj)
	$(ARMCC) $(DEFINES) $(LOCAL_LDLIBS)  imu_bmi160_test.c -o imu_bmi160_test
	adb push ./imu_bmi160_test /data/local



clean:
	rm -f *.o imu_test imu_bmi160_test spinal_tap stupid_hal a.out
