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

.PHONY: clean $(VERSION_HEADER) purgeAnkiLog verboseAnkiLog

all: k02 esp sim

$(ANKI_LOG_TABLE_JSON): $(ANKI_LOG_PP) $(shell /usr/bin/find $(ANKI_LOG_SRC_DIRS) -type f)
	python3 $(ANKI_LOG_PP) --preprocessor --string-table --output $@ $(ANKI_LOG_SRC_DIRS)

purgeAnkiLog: $(ANKI_LOG_PP)
	python3 $(ANKI_LOG_PP) --purge --preprocessor --string-table --output $(ANKI_LOG_TABLE_JSON) $(ANKI_LOG_SRC_DIRS)
	
verboseAnkiLog: $(ANKI_LOG_PP)
	python3 $(ANKI_LOG_PP) -vvv --preprocessor --string-table --output $(ANKI_LOG_TABLE_JSON) $(ANKI_LOG_SRC_DIRS)

$(VERSION_HEADER):
	$(VERSION_GENERATOR) $@

sim: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON)
	make fourzero -C clad -j4

nrf: $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

k02: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

esp: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4
	make -C espressif/app ROBOT_INC_DIR=$(shell pwd)/include BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3

espressif/bin/upgrade/user1.2048.new.3.bin: esp

$(BINARY_RELEASE_DIR)/esp.user.bin: espressif/bin/upgrade/user1.2048.new.3.bin
	cp $< $@

$(BINARY_RELEASE_DIR)/robot.safe: build/41/robot.safe
	cp $< $@

$(BINARY_RELEASE_DIR)/syscon.safe: syscon/build/syscon.safe
	cp $< $@

otaBinaries: $(BINARY_RELEASE_DIR)/esp.user.bin $(BINARY_RELEASE_DIR)/robot.safe $(BINARY_RELEASE_DIR)/syscon.safe

onBuildServer: k02 $(BINARY_RELEASE_DIR)/esp.user.bin


clean:
	make -C clad clean
	make -C espressif/app clean
	rm $(VERSION_HEADER)
