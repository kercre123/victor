
VICOS_BUILD_TOOL_VER:=0.9-r03


VICOS_ARM_TOOLS_ROOT:=~/.anki/vicos-sdk/dist/$(VICOS_BUILD_TOOL_VER)

VICOS_ARM_TOOLS_BIN:=$(VICOS_ARM_TOOLS_ROOT)/prebuilt/bin

# Compilation Tools

CROSS_COMPILE = $(VICOS_ARM_TOOLS_BIN)/arm-oe-linux-gnueabi-

CFLAGS = -c -fPIC -Wall -Werror


# Compilation Tools
CC := $(CROSS_COMPILE)clang

# Compiler flags
# Specify all the flags below which you want to use for your compilation, For this simple example, we only need to specify the include directory path
CFLAGS          += -I.
CFLAGS          += -I../../generated/clad/robot
CFLAGS          += -I../../syscon
CFLAGS          += -DPLATFORM_ANDROID=1


DEPS = ../spine/spine_hal.h ../spine/spine_crc.h
SRCS = ../spine/spine_hal.c ../spine/spine_crc.c dfu.c
OBJ = ../spine/spine_hal.o ../spine/spine_crc.o dfu.o

APP = dfu

all: $(VICOS_ARM_TOOLS_BIN) $(APP)

$(VICOS_ARM_TOOLS_BIN):
	../../../tools/build/tools/ankibuild/vicos.py --install $(VICOS_BUILD_TOOL_VER)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CROSS_COMPILE)clang $(CFLAGS) $<  -o $@


$(APP):  $(OBJ)
	$(CC) -pie -llog $(OBJ) -o $(APP)

clean:
	rm -f $(APP) *.o ../spine/*.o
