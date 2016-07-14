#
# Makefile for rBoot
#

SDK_BASE ?= /opt/esp-open-sdk/sdk

UPGRADER_BUILD_BASE ?= build
UPGRADER_FW_BASE    ?= firmware

CC := xtensa-lx106-elf-gcc
LD := xtensa-lx106-elf-gcc
OBJCOPY = xtensa-lx106-elf-objcopy
OBJDUMP = xtensa-lx106-elf-objdump

CFLAGS    = -Os -Wpointer-arith -Wundef -Werror -Wall -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH
LDFLAGS   = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static -L $(SDK_BASE)/ld/
LD_SCRIPT = upgrader.ld

BOOT=2
SPI_FREQ_DIV?=15 # 80MHz
SPI_MODE?=0      # QSPI
SPI_SIZE_MAP?=3  # 1MB images
flash?=2048      # 2MB total flash
addr?=0x00000    # Bootloader goes at the very beginning of flash

all: $(UPGRADER_BUILD_BASE) $(UPGRADER_FW_BASE) $(UPGRADER_FW_BASE)/upgrader.bin

$(UPGRADER_BUILD_BASE):
	mkdir -p $@

$(UPGRADER_FW_BASE):
	mkdir -p $@

$(UPGRADER_BUILD_BASE)/upgrader.o: upgrader.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -I$(UPGRADER_BUILD_BASE) $(UPGRADER_EXTRA_INCDIR) -c $< -o $@

$(UPGRADER_BUILD_BASE)/%.o: %.c %.h
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(UPGRADER_BUILD_BASE)/%.elf: $(UPGRADER_BUILD_BASE)/%.o
	@echo "LD $@"
	@$(LD) -T$(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $^ -Wl,--end-group -Xlinker --Map=esp.map -o $@

$(UPGRADER_FW_BASE)/%.bin: $(UPGRADER_BUILD_BASE)/%.elf
	@echo "GEN APPBIN $@"
	@$(OBJCOPY) --only-section .text       -O binary $< eagle.app.v6.text.bin
	@$(OBJCOPY) --only-section .data       -O binary $< eagle.app.v6.data.bin
	@$(OBJCOPY) --only-section .rodata     -O binary $< eagle.app.v6.rodata.bin
	@$(OBJCOPY) --only-section .irom0.text -O binary $< eagle.app.v6.irom0text.bin
	@$(OBJCOPY) --only-section .iram2.text -O binary $< eagle.app.v6.iram2text.bin
	@python2.7 $(SDK_BASE)/tools/gen_appbin.py $< $(BOOT) $(SPI_MODE) $(SPI_FREQ_DIV) $(SPI_SIZE_MAP)
	@mv eagle.app.flash.bin "$@"
	@mv eagle.app.v6.*.bin build/

clean:
	@echo "RM $(UPGRADER_BUILD_BASE) $(UPGRADER_FW_BASE)"
	@rm -rf $(UPGRADER_BUILD_BASE)
	@rm -rf $(UPGRADER_FW_BASE)
