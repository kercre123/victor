VICOS_BUILD_TOOL_VER:=0.9-r03


VICOS_ARM_TOOLS_ROOT:=~/.anki/vicos-sdk/dist/$(VICOS_BUILD_TOOL_VER)

VICOS_ARM_TOOLS_BIN:=$(VICOS_ARM_TOOLS_ROOT)/prebuilt/bin

# Compilation Tools

CROSS_COMPILE = $(VICOS_ARM_TOOLS_BIN)/arm-oe-linux-gnueabi-

CFLAGS = -c -fPIC -Wall


# All the c files in this directory will be compiled
SRCDIR = src
# Look for includes here
INCDIR = inc
# .o and other temp files go here
OBJDIR = .output

# the library we are building
LIB = libcore.a


SRCS = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)


all: $(VICOS_ARM_TOOLS_ROOT) $(LIB)

$(LIB): $(OBJECTS)
	$(CROSS_COMPILE)ar rcs $(LIB) $(OBJECTS)


$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CROSS_COMPILE)clang $(CFLAGS) $< -I$(INCDIR) -o $@


clean:
	-rm -f $(LIB) $(OBJECTS)
