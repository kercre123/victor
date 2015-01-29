scp vm:dev_bsp/rootfs/boot/uImage .\uImage
scp .\uImage cozmo:uImage
ssh --% cozmo "flash_eraseall /dev/mtd3 && nandwrite -p /dev/mtd3 uImage && /sbin/reboot"
