## @file Robot Firmware Makefile for Cozmo
# @author Daniel Casner
#
# Initially only handling Espressif firmware and pre/post-build steps for K02 and nrf51. May eventually have targets for
# all firmware.

ANKI_LOG_PP=tools/ankiLogPP.py
ANKI_LOG_TABLE_JSON=../resources/config/basestation/AnkiLogStringTables.json
ANKI_LOG_SRC_DIRS=supervisor/src k02_hal espressif/app sim_hal/ syscon/hal

BINARY_RELEASE_DIR=releases

VERSION_GENERATOR=tools/versionGenerator/versionGenerator.sh
VERSION_HEADER=include/anki/cozmo/robot/version.h

.PHONY: clean $(VERSION_HEADER) purgeAnkiLog verboseAnkiLog esp k02 nrf sim fresh

all: k02 esp sim

# Convenience rule
fresh: clean $(ANKI_LOG_TABLE_JSON) all python
	rm -Rf build ./syscon/build

$(ANKI_LOG_TABLE_JSON): $(ANKI_LOG_PP) $(shell /usr/bin/find $(ANKI_LOG_SRC_DIRS) -type f)
	python3 $(ANKI_LOG_PP) --preprocessor --string-table --output $@ $(ANKI_LOG_SRC_DIRS)

purgeAnkiLog: $(ANKI_LOG_PP)
	python3 $(ANKI_LOG_PP) --purge --preprocessor --string-table --output $(ANKI_LOG_TABLE_JSON) $(ANKI_LOG_SRC_DIRS)
	
verboseAnkiLog: $(ANKI_LOG_PP)
	python3 $(ANKI_LOG_PP) -vvv --preprocessor --string-table --output $(ANKI_LOG_TABLE_JSON) $(ANKI_LOG_SRC_DIRS)

$(VERSION_HEADER):
	$(VERSION_GENERATOR) $@

python:
	make -C clad python

app: $(ANKI_LOG_TABLE_JSON)

sim: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON)
	make fourzero -C clad -j4

nrf: $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

k02: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

k02_fix:
	python3 ./tools/boot_header.py ./build/app/robot.axf

nrf_fix:
	python3 ./tools/boot_header.py ./syscon/build/app/syscon.axf

# This fixes the headers and places the binaries in release
stage_binaries: k02_fix nrf_fix
	cp ./build/app/robot.axf ./binaries
	cp ./syscon/build/app/syscon.axf ./binaries
	cp ./espressif/bin/upgrade/user1.2048.new.3.bin ./binaries/esp.user.bin

stage: stage_binaries
	python3 ./tools/sign.py --rtip ./binaries/robot.axf 	./staging/rtip.safe
	python3 ./tools/sign.py --body ./binaries/syscon.axf	./staging/body.safe
	python3 ./tools/sign.py --wifi ./binaries/esp.user.bin 	./staging/wifi.safe
	python3 ./tools/sign.py --rtip ./binaries/robot.axf  --body ./binaries/syscon.axf --wifi ./binaries/esp.user.bin ./staging/cozmo.safe

release: stage_binaries
	#  --wifi ./binaries/esp.user.bin Should be added once single file OTA is finished
	python3 ./tools/sign.py --rtip ./binaries/robot.axf  --body ./binaries/syscon.axf ./releases/cozmo.safe
	cp ./binaries/esp.user.bin releases

kevin: k02_fix
	python3 ./tools/sign.py --rtip ./build/app/robot.axf ./staging/rtip.safe

vesp:
	vagrant ssh -c "make -C /vagrant/robot esp"

esp: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4
	make -C espressif/app ROBOT_INC_DIR=$(shell pwd)/include BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3

espressif/bin/upgrade/user1.2048.new.3.bin: esp

$(BINARY_RELEASE_DIR)/esp.user.bin: espressif/bin/upgrade/user1.2048.new.3.bin
	cp $< $@

$(BINARY_RELEASE_DIR)/cozmo.safe: k02_fix
	python3 ./tools/sign.py --rtip build/app/robot.axf  --body ./binaries/syscon.axf $@

otaBinaries: $(BINARY_RELEASE_DIR)/esp.user.bin $(BINARY_RELEASE_DIR)/cozmo.safe

onBuildServer: k02 $(BINARY_RELEASE_DIR)/esp.user.bin


clean:
	make -C clad clean
	make -C espressif/app clean
	rm -f $(VERSION_HEADER)
