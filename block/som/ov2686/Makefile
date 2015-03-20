obj-m := ov2686.o
KDIR  := ../rpm/BUILD/linux-3.0
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi-