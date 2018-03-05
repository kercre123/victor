ANDROID_NDK_ROOT:=~/.anki/android/ndk-repository/android-ndk-r15b

ANDROID_ARM_TOOLS_ROOT:=../../../generated/android/tools/arm
ANDROID_ARM_TOOLS_BIN:=$(ANDROID_ARM_TOOLS_ROOT)/bin
# Compilation Tools

CROSS_COMPILE = $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-
CFLAGS = -c -fPIC -Wall -pie


CROSS_COMPILE = $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-


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


# All the c files in this directory will be compiled
SRCDIR = .
# .o and other temp files go here
OBJDIR = .output

DFU = dfu
DFU_SRC = dfu.c
DFU_OBJ = $(DFU_SRC:%.c=$(OBJDIR)/%.o)

ANIMATE = animate
ANIMATE_SRC = animate.c
ANIMATE_OBJ = $(ANIMATE_SRC:%.c=$(OBJDIR)/%.o)

DISPLAY = display
DISPLAY_SRC = display.c helper_text.c display_test.c
DISPLAY_OBJ = $(DISPLAY_SRC:%.c=$(OBJDIR)/%.o)

DISPLAY_BLIT = display_blit
DISPLAY_BLIT_SRC = display.c display_blit_test.c
DISPLAY_BLIT_OBJ = $(DISPLAY_BLIT_SRC:%.c=$(OBJDIR)/%.o)

HELPER = helper
HELPER_SRC = main.c display.c helper_text.c logging.c pidopen.c
HELPER_OBJ = $(HELPER_SRC:%.c=$(OBJDIR)/%.o)

PIDOPEN = pidopen
PIDOPEN_SRC = pidopen.c
PIDOPEN_OBJ = $(PIDOPEN_SRC:%.c=$(OBJDIR)/%.o)


REMOTE_DEST = /data/local/fixture/

all: $(ANDROID_ARM_TOOLS_ROOT) $(DFU) $(ANIMATE) $(DISPLAY) $(HELPER)

$(CORE):
	cd $(CORE_DIR) && make all

$(ANDROID_ARM_TOOLS_ROOT):
	$(ANDROID_NDK_ROOT)/build/tools/make_standalone_toolchain.py --arch arm --install-dir $(ANDROID_ARM_TOOLS_ROOT) --api 24

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CROSS_COMPILE)gcc $(CFLAGS) $<  -o $@


$(DFU): $(CORE) $(DFU_OBJ)
	$(CROSS_COMPILE)gcc -pie $(DFU_OBJ) $(LIBPATH) $(LIBS)  -o $(DFU)

$(ANIMATE): $(CORE) $(ANIMATE_OBJ)
	$(CROSS_COMPILE)gcc -pie $(ANIMATE_OBJ) $(LIBPATH) $(LIBS)  -o $(ANIMATE)

$(DISPLAY): $(CORE) $(DISPLAY_OBJ)
	$(CROSS_COMPILE)gcc -pie $(DISPLAY_OBJ) $(LIBPATH) $(LIBS)  -o $(DISPLAY)

$(DISPLAY_BLIT): $(CORE) $(DISPLAY_BLIT_OBJ)
	$(CROSS_COMPILE)gcc -pie $(DISPLAY_BLIT_OBJ) $(LIBPATH) $(LIBS)  -o $(DISPLAY_BLIT)

$(HELPER): $(CORE) $(HELPER_OBJ)
	$(CROSS_COMPILE)gcc -pie $(HELPER_OBJ) $(LIBPATH) $(LIBS)  -o $(HELPER)

$(PIDOPEN): $(CORE) $(PIDOPEN_OBJ)
	$(CROSS_COMPILE)gcc -pie $(PIDOPEN_OBJ) $(LIBPATH) $(LIBS)  -o $(PIDOPEN)


app=$(HELPER)

push: $(app)
	adb shell -x "mkdir -p $(REMOTE_DEST)"
	adb push $(app)  $(REMOTE_DEST)

execute: $(app)
	adb shell -x "cd $(REMOTE_DEST) && ./$(app) $(safe)"

clean:
	rm -f $(APP) $(OBJDIR)/*.o
