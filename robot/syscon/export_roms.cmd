python ../tools/hex2bin.py 0 2000 ./components/softdevice/s110/hex/s110_softdevice.hex ./build/s110_low.bin
python ../tools/hex2bin.py 2000 18000 ./components/softdevice/s110/hex/s110_softdevice.hex ./build/s110_high.bin
python ../tools/hex2bin.py 1F000 20000 ./build/boot/sys_boot.hex ./build/bootloader.bin
