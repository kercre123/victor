ANDROID_TOOLCHAIN_DIR ?= $(shell pwd)/toolchain
ANDROID_API ?= 21
ANDROID_SYSROOT = $(ANDROID_NDK_ROOT)/platforms/android-$(ANDROID_API)/arch-arm

all: toolchain axattr

toolchain:
	$(ANDROID_NDK_ROOT)/build/tools/make_standalone_toolchain.py \
		--api=$(ANDROID_API) --install-dir=$(ANDROID_TOOLCHAIN_DIR) \
		--arch=arm --stl libc++
all: axattr 

TINYATTROBJS = axattr.o
axattr:  $(TINYATTROBJS)
	$(ANDROID_TOOLCHAIN_DIR)/bin/arm-linux-androideabi-clang \
	-o axattr $(TINYATTROBJS) -Os -pie -fPIE
%.o : %.c
	$(ANDROID_TOOLCHAIN_DIR)/bin/arm-linux-androideabi-clang \
	-c $(CFLAGS) -DNDEBUG -std=c99 -Os -o $@ $<
clean:
	rm -f \#* *~ *.o axattr 

clobber: clean
	rm -rf $(ANDROID_TOOLCHAIN_DIR)

