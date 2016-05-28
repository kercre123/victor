## @file Robot Firmware Makefile for Cozmo
# @author Daniel Casner
#
# Initially only handling Espressif firmware and pre/post-build steps for K02 and nrf51. May eventually have targets for
# all firmware.

ANKI_LOG_PP=tools/ankiLogPP.py
ANKI_LOG_TABLE_JSON=../resources/config/basestation/AnkiLogStringTables.json
ANKI_LOG_SRC_DIRS=supervisor/src k02_hal espressif/app sim_hal/ syscon/hal

STAGING_DIR=staging
BINARY_DIR=binaries
RELEASE_DIR=releases

VERSION_GENERATOR=tools/versionGenerator/versionGenerator.sh
VERSION_HEADER=include/anki/cozmo/robot/version.h
DEVELOPMENT_KEY=tools/development.pem

.PHONY: clean $(VERSION_HEADER) purgeAnkiLog verboseAnkiLog esp k02 nrf sim fresh app

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

staging_dir:
	mkdir -p ./staging

export_key:
	mkdir -p generated
	python3 ./tools/export_public.py -k $(DEVELOPMENT_KEY) ./generated/publickey.h
	g++ -o generated/calcMont ./tools/calcMontgomery.cpp ../crypto/bignum.cpp -I ../crypto -I generated
	./generated/calcMont >> ./generated/publickey.h

k02_fix: staging_dir
	python3 ./tools/boot_header.py ./build/app/robot.axf
	python3 ./tools/sign.py --wifi ./binaries/esp.user.bin --rtip ./build/app/robot.axf  --body ./binaries/syscon.axf ./staging/cozmo.safe

k02_fcc_fix: staging_dir
	python3 ./tools/boot_header.py ./build/fcc/robot.axf

nrf_fix:
	python3 ./tools/boot_header.py ./syscon/build/app/syscon.axf

# This fixes the headers and places the binaries in release
stage_binaries:
	@if test ./build/app/robot.axf -nt ./binaries/robot.axf; then cp -v ./build/app/robot.axf ./binaries/robot.axf; fi
	@if test ./syscon/build/app/syscon.axf -nt ./binaries/syscon.axf; then cp -v ./syscon/build/app/syscon.axf ./binaries/syscon.axf; fi
	@if test ./staging/esp.user.bin -nt ./binaries/esp.user.bin; then cp -v ./staging/esp.user.bin ./binaries/esp.user.bin; fi

release: stage_binaries
	python3 ./tools/sign.py --wifi binaries/esp.user.bin --rtip ./binaries/robot.axf  --body ./binaries/syscon.axf ./releases/cozmo.safe
	#python3 ./tools/sign.py -c --sign ./tools/development.pem --wifi binaries/esp.user.bin --rtip ./binaries/robot.axf  --body ./binaries/syscon.axf ./releases/cozmo.signed.safe

vesp:
	vagrant ssh -c "make -C /vagrant/robot esp"

esp: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON) export_key staging_dir
	make $@ -C clad -j4
	make -C espressif/app ROBOT_INC_DIR=$(shell pwd)/include BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3
	cp ./espressif/bin/upgrade/user1.2048.new.3.bin ./staging/esp.user.bin
	python3 ./tools/sign.py --wifi ./staging/esp.user.bin --rtip ./binaries/robot.axf  --body ./binaries/syscon.axf ./staging/cozmo.safe

onBuildServer: esp


clean:
	make -C clad clean
	make -C espressif/app clean
	rm -f $(VERSION_HEADER)
