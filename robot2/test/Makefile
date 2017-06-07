
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

all: test

app: 
	gcc $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) *.cpp

test: app
	./a.out -c

%.o: %.c $(DEPS)
	$(CC)  -I../spine -c -o $@ $< $(CFLAGS)


stupid:
	echo $(PLATFORM)
#	$(CC) -v $(DEFINES) $(LOCAL_LDLIBS) -gcc-toolchain -L../spine -lspine stupid_hal.c
	$(CC) $(DEFINES) $(LOCAL_LDLIBS) -I../spine ../spine/spine_hal.c ../spine/spine_crc.c stupid_hal.c 


