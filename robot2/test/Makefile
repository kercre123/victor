
CPPUTEST_HOME = /usr/local/Cellar/cpputest/3.8

DEFINES += -DCOZMO_ROBOT

CPPFLAGS += -I$(CPPUTEST_HOME)
CPPFLAGS += -I"../../robot/include"
CPPFLAGS += -I"../hal/include"
CPPFLAGS += $(DEFINES)

CXXFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorNewMacros.h
CFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorMallocMacros.h

LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt
LDFLAGS += -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt -lc++

ifeq ($(PLATFORM),OSX)
	DEFINES += -DPLATFORM_OSX
  CC = gcc
else
	DEFINES += -DPLATFORM_ANDROID
  CC = ~/arm/bin/clang -pie
	LOCAL_LDLIBS += -llog 
endif

#android arm build toolchain
ANDROID_NDK_ROOT:=~/.anki/android/ndk-repository/android-ndk-r15b

ANDROID_ARM_TOOLS_ROOT:=../../generated/android/tools/arm
ANDROID_ARM_TOOLS_BIN:=$(ANDROID_ARM_TOOLS_ROOT)/bin
# Compilation Tools
ARMCC := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-gcc
ARMCPP := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-g++
ARMAR := $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-ar



$(ANDROID_ARM_TOOLS_ROOT):
	$(ANDROID_NDK_ROOT)/build/tools/make_standalone_toolchain.py --arch arm --install-dir $(ANDROID_ARM_TOOLS_ROOT) --api 24

lcd_test:  $(ANDROID_ARM_TOOLS_ROOT) lcd_test.c
	$(ARMCC) -pie -llog lcd_test.c -o lcd_test



app: 
	gcc $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) *.cpp

test: app
	./a.out -c

%.o: %.c $(DEPS)
	$(CC)  -I../spine -c -o $@ $< $(CFLAGS)


stupid:
	echo $(PLATFORM)
#	$(CC) -v $(DEFINES) $(LOCAL_LDLIBS) -gcc-toolchain -L../spine -lspine stupid_hal.c
	$(CC) $(DEFINES) $(LOCAL_LDLIBS) -I../hal/spine -I../syscon ../hal/spine/spine_hal.c ../hal/spine/spine_crc.c stupid_hal.c 


spinal_tap:
#	$(CC) -v $(DEFINES) $(LOCAL_LDLIBS) -gcc-toolchain -L../spine -lspine stupid_hal.c
	$(CC) $(DEFINES) $(LOCAL_LDLIBS) -I../hal/spine -I../syscon  ../hal/spine/spine_hal.c ../hal/spine/spine_crc.c spinal_tap.c -o spinal_tap


