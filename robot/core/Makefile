ANDROID_NDK_ROOT:=~/.anki/android/ndk-repository/android-ndk-r15b

ANDROID_ARM_TOOLS_ROOT:=../../generated/android/tools/arm
ANDROID_ARM_TOOLS_BIN:=$(ANDROID_ARM_TOOLS_ROOT)/bin

CROSS_COMPILE = $(ANDROID_ARM_TOOLS_BIN)/arm-linux-androideabi-

CFLAGS = -c -fPIC -Wall -pie

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


all: $(ANDROID_ARM_TOOLS_ROOT) $(LIB)

$(LIB): $(OBJECTS)
	$(CROSS_COMPILE)ar rcs $(LIB) $(OBJECTS)


$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CROSS_COMPILE)gcc $(CFLAGS) $< -I$(INCDIR) -o $@


clean:
	-rm -f $(LIB) $(OBJECTS)
