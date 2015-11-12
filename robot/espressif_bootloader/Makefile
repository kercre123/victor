#
# Makefile for rBoot
#

SDK_BASE ?= /opt/esp-open-sdk/sdk

RBOOT_BUILD_BASE ?= build
RBOOT_FW_BASE    ?= firmware

CC := xtensa-lx106-elf-gcc
LD := xtensa-lx106-elf-gcc
OBJCOPY = xtensa-lx106-elf-objcopy
OBJDUMP = xtensa-lx106-elf-objdump

CFLAGS    = -Os -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH
LDFLAGS   = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static -L $(SDK_BASE)/ld/
LD_SCRIPT = rboot.ld

BOOT=0
SPI_FREQ_DIV?=15
SPI_MODE?=0
SPI_SIZE_MAP?=3
flash?=2048
addr?=0x00000

all: $(RBOOT_BUILD_BASE) $(RBOOT_FW_BASE) $(RBOOT_FW_BASE)/rboot.bin

$(RBOOT_BUILD_BASE):
	mkdir -p $@

$(RBOOT_FW_BASE):
	mkdir -p $@

$(RBOOT_BUILD_BASE)/rboot-stage2a.o: rboot-stage2a.c rboot-private.h rboot.h
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(RBOOT_EXTRA_INCDIR) -c $< -o $@

$(RBOOT_BUILD_BASE)/rboot.o: rboot.c rboot-private.h rboot.h
	@echo "CC $<"
	@$(CC) $(CFLAGS) -I$(RBOOT_BUILD_BASE) $(RBOOT_EXTRA_INCDIR) -c $< -o $@

$(RBOOT_BUILD_BASE)/%.o: %.c %.h
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(RBOOT_BUILD_BASE)/%.elf: $(RBOOT_BUILD_BASE)/%.o
	@echo "LD $@"
	@$(LD) -T$(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $^ -Wl,--end-group -Xlinker --Map=esp.map -o $@

$(RBOOT_FW_BASE)/%.bin: $(RBOOT_BUILD_BASE)/%.elf
	@echo "GEN APPBIN $@"
	@$(OBJCOPY) --only-section .text       -O binary $< eagle.app.v6.text.bin
	@$(OBJCOPY) --only-section .data       -O binary $< eagle.app.v6.data.bin
	@$(OBJCOPY) --only-section .rodata     -O binary $< eagle.app.v6.rodata.bin
	@$(OBJCOPY) --only-section .irom0.text -O binary $< eagle.app.v6.irom0text.bin
	@$(OBJCOPY) --only-section .iram2.text -O binary $< eagle.app.v6.iram2text.bin
	@python2 $(SDK_BASE)/tools/gen_appbin.py $< $(BOOT) $(SPI_MODE) $(SPI_FREQ_DIV) $(SPI_SIZE_MAP)
	@mv eagle.app.flash.bin "$@"
	#@rm eagle.app.v6.*.bin

clean:
	@echo "RM $(RBOOT_BUILD_BASE) $(RBOOT_FW_BASE)"
	@rm -rf $(RBOOT_BUILD_BASE)
	@rm -rf $(RBOOT_FW_BASE)
