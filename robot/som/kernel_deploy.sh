TARGET=$1
scp vm:dev_bsp/rootfs/boot/uImage uImage
scp uImage $TARGET:
ssh $TARGET "flash_eraseall /dev/mtd3 && nandwrite -p /dev/mtd3 uImage && /sbin/reboot"
