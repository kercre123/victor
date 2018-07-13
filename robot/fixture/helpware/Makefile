
VICOS_BUILD_TOOL_VER:=0.9-r03


VICOS_ARM_TOOLS_ROOT:=~/.anki/vicos-sdk/dist/$(VICOS_BUILD_TOOL_VER)

VICOS_ARM_TOOLS_BIN:=$(VICOS_ARM_TOOLS_ROOT)/prebuilt/bin

# Compilation Tools

CROSS_COMPILE = $(VICOS_ARM_TOOLS_BIN)/arm-oe-linux-gnueabi-

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
HELPER_SRC = main.c display.c helper_text.c logging.c pidopen.c kbhit.c
HELPER_OBJ = $(HELPER_SRC:%.c=$(OBJDIR)/%.o)

PIDOPEN = pidopen
PIDOPEN_SRC = pidopen.c
PIDOPEN_OBJ = $(PIDOPEN_SRC:%.c=$(OBJDIR)/%.o)

COMMANDER = commander
COMMANDER_SRC = commander.c kbhit.c display.c
COMMANDER_OBJ = $(COMMANDER_SRC:%.c=$(OBJDIR)/%.o) $(SPINE_OBJ)

SPINE_SRC = spine_hal.c spine_crc.c
SPINE_OBJ = $(SPINE_SRC:%.c=$(OBJDIR)/%.o)

MENUMAN = menuman
MENUMAN_SRC = menuman.c kbhit.c display.c pidopen.c
MENUMAN_OBJ = $(MENUMAN_SRC:%.c=$(OBJDIR)/%.o) $(SPINE_OBJ)

BURN = burn
BURN_SRC = burn.c
BURN_OBJ = $(BURN_SRC:%.c=$(OBJDIR)/%.o)

REMOTE_DEST = /data/local/fixture/

all: $(VICOS_ARM_TOOLS_BIN) $(DFU) $(ANIMATE) $(DISPLAY) $(HELPER) $(MENUMAN) $(BURN)

$(CORE):
	cd $(CORE_DIR) && make all

$(VICOS_ARM_TOOLS_BIN):
	../../../tools/build/tools/ankibuild/vicos.py --install $(VICOS_BUILD_TOOL_VER)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CROSS_COMPILE)clang $(CFLAGS) $<  -o $@


$(DFU): $(CORE) $(DFU_OBJ)
	$(CROSS_COMPILE)clang -pie $(DFU_OBJ) $(LIBPATH) $(LIBS)  -o $(DFU)

$(ANIMATE): $(CORE) $(ANIMATE_OBJ)
	$(CROSS_COMPILE)clang -pie $(ANIMATE_OBJ) $(LIBPATH) $(LIBS)  -o $(ANIMATE)

$(DISPLAY): $(CORE) $(DISPLAY_OBJ)
	$(CROSS_COMPILE)clang -pie $(DISPLAY_OBJ) $(LIBPATH) $(LIBS)  -o $(DISPLAY)

$(DISPLAY_BLIT): $(CORE) $(DISPLAY_BLIT_OBJ)
	$(CROSS_COMPILE)gcc -pie $(DISPLAY_BLIT_OBJ) $(LIBPATH) $(LIBS)  -o $(DISPLAY_BLIT)

$(HELPER): $(CORE) $(HELPER_OBJ)
	$(CROSS_COMPILE)clang -pie $(HELPER_OBJ) $(LIBPATH) $(LIBS)  -o $(HELPER)

$(BURN): $(CORE) $(BURN_OBJ)
	$(CROSS_COMPILE)clang -pie $(BURN_OBJ) $(LIBPATH) $(LIBS)  -o $(BURN)

$(COMMANDER): $(CORE) $(COMMANDER_OBJ)
	$(CROSS_COMPILE)clang -pie $(COMMANDER_OBJ) $(LIBPATH) $(LIBS) -llog -o $(COMMANDER)

$(OBJDIR)/spine_hal.o:
	$(CROSS_COMPILE)clang $(CFLAGS) ../../hal/spine/spine_hal.c  -o $@
$(OBJDIR)/spine_crc.o:
	$(CROSS_COMPILE)clang $(CFLAGS) ../../hal/spine/spine_crc.c  -o $@


$(MENUMAN): $(CORE) $(MENUMAN_OBJ)
	$(CROSS_COMPILE)clang -pie $(MENUMAN_OBJ) $(LIBPATH) $(LIBS) -llog  -o $(MENUMAN)


app=$(HELPER)

push: $(app)
	adb shell -x "mkdir -p $(REMOTE_DEST)"
	adb push $(app)  $(REMOTE_DEST)

execute: $(app)
	adb shell -x "cd $(REMOTE_DEST) && ./$(app) $(safe)"

clean:
	rm -f $(APP) $(OBJDIR)/*.o
