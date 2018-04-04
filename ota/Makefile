IMG_DIR=../poky/build/tmp-glibc/deploy/images/apq8009-robot-robot
BUILD=_build
BOOT_STEM=apq8009-robot-boot
SYS_STEM=apq8009-robot-sysfs
MANIFEST=$(BUILD)/manifest
OTA_TAR=$(BUILD)/ota.tar

BOOT_GZ=$(BUILD)/$(BOOT_STEM).img.gz
BOOT_STATS=$(BUILD)/$(BOOT_STEM).stats
SYS_GZ=$(BUILD)/$(SYS_STEM).img.gz
SYS_STATS=$(BUILD)/$(SYS_STEM).stats

GZIP_MODE=--best
GZIP_WBITS=31

OTAKEY ?= ota_test.key

all: $(BUILD) $(OTA_TAR)

clean:
	rm -rf $(BUILD)

deepclean: clean
	rm -f $(IMG_DIR)/*

.PHONY: clean deepclean hlos fbc

.PRECIOUS: $(BUILD)/%.img

hlos:
	cd common/build; python2 update_common_info.py --hlos

fbc:
	cd common/build; python2 fastboot_complete.py
	fastboot erase userdata
	fastboot erase boot_a
	fastboot erase boot_b
	fastboot flash oem factory.img

$(BUILD):
	mkdir $(BUILD)

$(BUILD)/%.img: $(IMG_DIR)/%.img
	cp $< $@

%.img.gz: %.img
	gzip $(GZIP_MODE) --force --keep $<

$(BUILD)/%.img: $(IMG_DIR)/%.ext4
	simg2img $< $@

%.stats: %.img
	echo "bytes=`wc -c $< | awk '{ print $$1 }'`"            > $@
	echo "sha256=`shasum -a256 -b $< | awk '{ print $$1 }'`" >> $@

%.sha256: %.ini
	openssl dgst -sha256 -sign $(OTAKEY) -out $@ $<

$(MANIFEST).ini: $(BUILD) $(BOOT_STATS) $(SYS_STATS)
	echo "[META]"                  > $@
	echo "manifest_version=0.9.2" >> $@
	echo "update_version=0.8.1"   >> $@
	echo "num_images=2"           >> $@
	echo "[BOOT]"                 >> $@
	echo "delta=0"                >> $@
	echo "compression=gz"         >> $@
	echo "wbits=$(GZIP_WBITS)"    >> $@
	cat $(BOOT_STATS)             >> $@
	echo "[SYSTEM]"               >> $@
	echo "delta=0"                >> $@
	echo "compression=gz"         >> $@
	echo "wbits=$(GZIP_WBITS)"    >> $@
	cat $(SYS_STATS)              >> $@


$(OTA_TAR): $(MANIFEST).sha256 $(BOOT_GZ) $(SYS_GZ)
	tar cf $@ -C $(BUILD) manifest.ini manifest.sha256 apq8009-robot-boot.img.gz apq8009-robot-sysfs.img.gz

serve: $(OTA_TAR)
	cd $(BUILD); python -m SimpleHTTPServer 5555
