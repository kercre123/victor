#
# Common variables and recipies used by Makefiles.
# The following variables are defined:
#  CC: C cross-compiler
#  CPP: cross-preprocessor
#  OBJCOPY: cross-objcopy
#  USE_NANO: link flags to select newlib-nano
#  USE_SEMIHOST: link flags to enable semihosting
#  USE_NOHOST: link flags to disable semihosting
#  MAP: link flags to create a map file
#  LDSCRIPTS: link flags to use a custom link script, generated from $(LINK_SCRIPT).S
#

CROSS_COMPILE = arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)cpp
OBJCOPY = $(CROSS_COMPILE)objcopy

# verbosity switch
V ?= 0
ifeq ($(V),0)
	V_CC = @echo "  CC    " $@;
	V_CPP = @echo "  CPP   " $@;
	V_LINK = @echo "  LINK  " $@;
	V_OBJCOPY = @echo "  OBJCPY" $@;
	V_CLEAN = @echo "  CLEAN ";
	V_SED = @echo "  SED   " $@;
	V_GAWK = @echo "  GAWK  " $@;
else
	V_OPT = '-v'
endif

# Use newlib-nano. To disable it, specify USE_NANO=
USE_NANO := --specs=nano.specs

# Use semihosting or not
USE_SEMIHOST := --specs=rdimon.specs
USE_NOHOST := --specs=nosys.specs

# Create map file
MAP = -Wl,-Map=$(O)/lst/$(TARGET_ELF).map

ARCH_FLAGS = -mthumb -mcpu=cortex-m$(CORTEX_M)

# general compilation flags
CFLAGS += $(ARCH_FLAGS) $(STARTUP_DEFS) -std=gnu99

ifeq ($(V),2)
	CFLAGS += --verbose
	LDFLAGS += -Wl,--verbose
endif

ROM_SYMDEF := rom.symdef
ROM_SYMBOLS := rom.symbols
LINK_SCRIPT := 580.lds

LDSCRIPTS := -L. -T $(LINK_SCRIPT)


# how to compile C files
%.o : %.c
	$(V_CC)$(CC) $(CFLAGS) -c $< -o $@

# how to compile assembly files
%.o : %.S
	$(V_CC)$(CC) $(CFLAGS) -c $< -o $@


all: $(O)/$(TARGET_ELF).hex $(O)/$(TARGET_ELF).bin

clean:
	$(V_CLEAN)for file in $(OBJ); do rm -f $(V_OPT) "$$file"; done
	@rm -rf $(V_OPT) $(O)
	@rm -f $(V_OPT) $(ROM_SYMDEF) $(ROM_SYMBOLS) $(LINK_SCRIPT)

$(O)/ :
	@mkdir -p $(V_OPT) $(O)/lst

# how to create the main ELF target
$(O)/$(TARGET_ELF).axf: $(O)/ $(ROM_SYMBOLS) $(LINK_SCRIPT) $(OBJ)
	$(V_LINK)$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) $(patch_objs) -o $@ 

# how to create the final hex file
$(O)/$(TARGET_ELF).hex: $(O)/$(TARGET_ELF).axf
	$(V_OBJCOPY)$(OBJCOPY) -O ihex $< $@

# how to create the final binary file
$(O)/$(TARGET_ELF).bin: $(O)/$(TARGET_ELF).axf
	$(V_OBJCOPY)$(OBJCOPY) -O binary $< $@

# how to create the linker script
$(LINK_SCRIPT): $(LINK_SCRIPT).S
	$(V_CPP)$(CPP) -P $< -o $@

# how to create a clean, sorted list of known symbols, used by ROM code
$(ROM_SYMDEF): $(ROM_MAP_FILE)
	$(V_SED)sed -n -e 's/  */ /g p' $< | sed -e '/^[;#]/d' | \
		sort | dos2unix > $@

# how to create a file with the known symbols, to be used in the linker script
$(ROM_SYMBOLS): $(ROM_SYMDEF)
	$(V_GAWK)gawk '{printf "%s = %s ;\n", $$3, $$1}' $< > $@


