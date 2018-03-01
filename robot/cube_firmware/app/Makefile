OBJECTS = accel.o board.o lights.o uart.o animation.o main.o ../sdk/platform/driver/gpio/gpio.o lib/gcc/__aeabi_idiv.o lib/gcc/__udivmodsi4.o lib/memcpy.o lib/memset.o lib/boot.o
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as

INCLUDES = -I../sdk/platform/include -I../sdk/platform/driver/gpio -I../sdk/platform/arch -I../sdk/platform/arch/compiler/rvds -I../sdk/platform/arch/ll/rvds -I../sdk/platform/driver/reg -I../src/config -I../sdk/platform/core_modules/rwip/api -I../sdk/common_project_files
CFLAGS = -O2 $(INCLUDES) -mcpu=cortex-m0

all: cube.dfu

clean:
	rm -Rf $(OBJECTS) cube.bin cube.elf cube.dfu

run: cube.dfu
	node ../bletool ./$<

cube.dfu: cube.bin
	python3 ../tools/sign.py $< $@

cube.bin: cube.elf
	arm-none-eabi-objcopy --output-target=binary $< $@

cube.elf: $(OBJECTS)
	arm-none-eabi-ld -T lib/linker.ls -o $@ $(OBJECTS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.s.o:
	$(AS) -mcpu=cortex-m0 -mthumb -o $@ $<

.phony: all clean run
