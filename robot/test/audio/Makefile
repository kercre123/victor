#
# Use android.py to locate Android NDK
#
TOPLEVEL := ../../..
ANDROID_PY := $(TOPLEVEL)/tools/build/tools/ankibuild/android.py
ANDROID_NDK_ROOT := $(shell $(ANDROID_PY))

ANDROID_ARM_TOOLS_ROOT := $(TOPLEVEL)/generated/android/tools/arm
ANDROID_ARM_TOOLS_BIN := $(ANDROID_ARM_TOOLS_ROOT)/bin

# Compilation Tools
# -c don't run linker, -fPIC emit position-independent code, -Wall enables all the warning
# -pie produce a dynamically linked position independent executable, -g include symbols, -O0 no optimization (default)
CFLAGS = -c -fPIC -Wall -pie -g -O0
INC = include
OBJECTS = mixer.o pcm.o
SHLIB = libtinyalsa.so
STLIB = libtinyalsa.a

LIB = $(STLIB)

CROSS_COMPILE = $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-

# all: $(ANDROID_ARM_TOOLS_ROOT) $(LIB) tinyplay tinycap tinymix tinypcminfo
all: $(ANDROID_ARM_TOOLS_ROOT) $(LIB) alsaplay

tinyplay: $(LIB) tinyplay.o
	$(CROSS_COMPILE)gcc -pie tinyplay.o -L. -ltinyalsa -o tinyplay

tinycap: $(LIB) tinycap.o
	$(CROSS_COMPILE)gcc tinycap.o -L. -ltinyalsa -o tinycap

tinymix: $(LIB) tinymix.o
	$(CROSS_COMPILE)gcc -pie tinymix.o -L. -ltinyalsa -o tinymix

tinypcminfo: $(LIB) tinypcminfo.o
	$(CROSS_COMPILE)gcc -pie tinypcminfo.o -L. -ltinyalsa -o tinypcminfo

alsaplay: $(LIB) alsaplay.o
	$(CROSS_COMPILE)gcc -pie alsaplay.o -L. -ltinyalsa -o alsaplay



$(SHLIB): $(OBJECTS)
	$(CROSS_COMPILE)gcc -shared $(OBJECTS) -o $(SHLIB)

$(STLIB): $(OBJECTS)
#	$(CROSS_COMPILE)gcc $(OBJECTS) 
	$(CROSS_COMPILE)ar rcs $(STLIB) $(OBJECTS) 

.c.o:
	$(CROSS_COMPILE)gcc $(CFLAGS) $< -I$(INC)

clean:
	-rm -f $(SHLIB) $(STLIB) $(OBJECTS) tinyplay.o tinyplay tinycap.o tinycap \
	tinymix.o tinymix tinypcminfo.o tinypcminfo alsaplay.o alsaplay 
