all : image.elf
FW_FILE_1:=0x00000.bin
FW_FILE_2:=0x40000.bin

TARGET_OUT:=image.elf
OBJS:=user/user_main.o user/client.o user/block_relay.o driver/uart.o

SRCS:=user/user_main.c user/client.c user/block_relay.c driver/uart.c

CWD := $(shell pwd)
GCC_FOLDER:=../../../coretech-external/espressif/xtensa-toolchain-build/build-lx106
ESPTOOL_PY:=../../../coretech-external/espressif/esptool/esptool.py
FW_TOOL:= ../../../coretech-external/espressif/other/esptool/esptool
SDK:= ../../../coretech-external/espressif/iot_sdk
PORT:=com7
BAUD:=230400
FLASH_SIZE:=16m

XTLIB:=$(SDK)/lib
XTGCCLIB:=$(GCC_FOLDER)/gcc-4.9.1-elf/xtensa-lx106-elf/libgcc/libgcc.a
FOLDERPREFIX:=$(GCC_FOLDER)/root/bin
PREFIX:=$(FOLDERPREFIX)/xtensa-lx106-elf-
CC:=$(PREFIX)gcc

LX106:=../../../coretech-external/espressif/lx106

CFLAGS:=-mlongcalls -I$(SDK)/include -Imyclib -Iinclude -Iuser -Os -I$(SDK)/include/

LDFLAGS_CORE:=\
	-nostdlib \
	-Wl,--relax -Wl,--gc-sections \
	-L$(XTLIB) \
	-L$(XTGCCLIB) \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libssl.a \
	$(SDK)/lib/libupgrade.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libwpa.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/libphy.a \
	$(SDK)/lib/libmain.a \
	$(SDK)/lib/libpp.a \
	$(LX106)/lib/libc.a \
	$(XTGCCLIB) \
	-T $(SDK)/ld/eagle.app.v6.ld

LINKFLAGS:= \
	$(LDFLAGS_CORE) \
	-B$(XTLIB)

$(TARGET_OUT) : $(SRCS)
	$(PREFIX)gcc $(CFLAGS) $^  -flto $(LINKFLAGS) -o $@

$(FW_FILE_1): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

$(FW_FILE_2): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -es .irom0.text $@ -ec

vm : $(SRCS)
	ssh l "cd Documents/GitHub/products-cozmo/robot/espressif && make build"

build : $(FW_FILE_1) $(FW_FILE_2)

burn-bench :
	($(ESPTOOL_PY) --port com7 --baud 460800 write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)

burn-hat :
	($(ESPTOOL_PY) --port com8 --baud 460800 write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)

burn-3 :
	($(ESPTOOL_PY) --port com11 --baud 460800 write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)

burn-4 :
	($(ESPTOOL_PY) --port com16 --baud $(BAUD) write_flash --flash_size $(FLASH_SIZE) 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)


clean :
	rm -rf user/*.o driver/*.o $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)
