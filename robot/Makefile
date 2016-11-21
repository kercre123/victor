## @file Robot Firmware Makefile for Cozmo
# @author Daniel Casner
#
# Initially only handling Espressif firmware and pre/post-build steps for K02 and nrf51. May eventually have targets for
# all firmware.

ANKI_LOG_PP=tools/ankiLogPP.py
ANKI_LOG_TABLE_JSON=../resources/config/basestation/AnkiLogStringTables.json
ANKI_LOG_SRC_DIRS=supervisor/src k02_hal espressif/app sim_hal/ syscon/hal

UV4=C:/Keil_v5/UV4/UV4.exe
STAGING_DIR=staging
BINARY_DIR=binaries
RELEASE_DIR=releases

VERSION_GENERATOR=tools/versionGenerator/versionGenerator.sh
VERSION_HEADER=include/anki/cozmo/robot/version.h

UNSIGNED_BINARY=$(RELEASE_DIR)/cozmo.unsafe
SIGNED_BINARY=$(RELEASE_DIR)/cozmo.safe
STAGING_BINARY=$(STAGING_DIR)/cozmo.safe
REVERT_BINARY=$(STAGING_DIR)/revert.safe

DIFFIE_KEY=certs/diffie.pem
DEVELOPMENT_KEY=certs/development.pem
AES_KEY=FA67CE138A376CD21A26118615326AA5
SIGN_FLAGS=--sign $(AES_KEY) $(DEVELOPMENT_KEY)

.PHONY: clean $(VERSION_HEADER) purgeAnkiLog verboseAnkiLog esp esp_factory k02 nrf sim fresh app export_key tests

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

nrf: export_key $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

k02: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

keil:
	$(UV4) -c ./cozmo.uvmpw
	$(UV4) -b ./cozmo.uvmpw

staging_dir:
	mkdir -p ./staging

export_key:
	mkdir -p generated build
	python3 ./tools/export_public.py -d $(DIFFIE_KEY) -k $(DEVELOPMENT_KEY) -a $(AES_KEY) ./generated/publickeys.h
	g++ -o build/calcMont ./tools/calcMontgomery.cpp ../crypto/bignum.cpp -I ../crypto -I generated
	build/calcMont >> ./generated/publickeys.h

crypto_files = ../crypto/bignum.cpp
sign_test_files = ./robot-test/signtest.cpp ../crypto/rsa_pss.cpp ../crypto/aes.cpp ../crypto/crc32.cpp ../crypto/sha512.cpp 
diffie_test_files = ./robot-test/diffietest.cpp ../crypto/sha1.cpp

tests: export_key $(sign_test_files)
	g++ -o ./build/diffietest $(diffie_test_files) $(crypto_files) -I ../crypto -I ./generated
	build/diffietest

	g++ -o ./build/signtest $(sign_test_files) $(crypto_files) -I ../crypto -I include/anki/cozmo/robot -I ./generated
	build/signtest $(UNSIGNED_BINARY) $(SIGNED_BINARY)

k02_fix: staging_dir
	python3 ./tools/boot_header.py ./build/app/robot.axf
	python3 ./tools/sign.py $(SIGN_FLAGS) --wifi ./binaries/esp.user.bin --rtip ./build/app/robot.axf  --body ./binaries/syscon.axf $(STAGING_BINARY)

k02_revert:
	python3 ./tools/boot_header.py ./build/app/revert.axf
	python3 ./tools/sign.py $(SIGN_FLAGS) --wifi ./binaries/esp.user.bin --rtip ./build/app/revert.axf --body ./binaries/syscon.axf $(REVERT_BINARY)

k02_fcc_fix: staging_dir
	python3 ./tools/boot_header.py ./build/fcc/robot.axf

nrf_fix:
	python3 ./tools/boot_header.py ./syscon/build/app/syscon.axf

# This fixes the headers and places the binaries in release
stage_binaries:
	@if test ./build/app/robot.axf -nt ./binaries/robot.axf; then cp -v ./build/app/robot.axf ./binaries/robot.axf; fi
	@if test ./syscon/build/app/syscon.axf -nt ./binaries/syscon.axf; then cp -v ./syscon/build/app/syscon.axf ./binaries/syscon.axf; fi
	@if test ./staging/esp.user.bin  -nt ./binaries/esp.user.bin; then cp -v ./staging/esp.user.bin  ./binaries/esp.user.bin; fi

# This is needed when timestamps get screwed up (as they constantly do with Vagrant/VM)
force_stage_binaries:
	cp -v ./build/app/robot.axf ./binaries/robot.axf
	cp -v ./syscon/build/app/syscon.axf ./binaries/syscon.axf
#	cp -v ./staging/esp.user.bin  ./binaries/esp.user.bin

stage_factory_upgrade:
	cp -v staging/esp.factory.bin binaries/esp.factory.bin
	cp -v staging/factory.safe    binaries/factory.safe

release: stage_binaries
	#if [ -a $(UV4) ]; then make keil; fi;
	python3 ./tools/sign.py -c --wifi binaries/esp.user.bin --rtip ./binaries/robot.axf  --body ./binaries/syscon.axf $(UNSIGNED_BINARY)
	python3 ./tools/sign.py $(SIGN_FLAGS) --wifi binaries/esp.user.bin --rtip ./binaries/robot.axf  --body ./binaries/syscon.axf $(SIGNED_BINARY)
	if test -e ./staging/factory_upgrade.safe; then cp -v ./staging/factory_upgrade.safe releases/; fi
	make tests

vesp:
	vagrant ssh -c "make -C /vagrant/robot esp"

esp: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON) export_key staging_dir
	make $@ -C clad -j4
	make -C espressif lib/libmain_w.a
	make -C espressif/app ROBOT_INC_DIR=$(shell pwd)/include BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3 LD_FILE=../ld/espressif_application.ld FACTORY_FIRMWARE=0
	python3 ./tools/wifiAppImageHeader.py ./espressif/bin/upgrade/user1.2048.new.3.bin ./staging/esp.user.bin
	python3 ./tools/sign.py $(SIGN_FLAGS) --wifi ./staging/esp.user.bin --rtip ./binaries/robot.axf  --body ./binaries/syscon.axf $(STAGING_BINARY)

esp_factory: $(VERSION_HEADER) $(ANKI_LOG_TABLE_JSON) staging_dir export_key
	make -C espressif clean
	cd espressif_bootloader; ./build.sh; cd ..
	make esp -C clad -j4
	make -C espressif/app ROBOT_INC_DIR=$(shell pwd)/include BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3 LD_FILE=../ld/espressif_factory.ld FACTORY_FIRMWARE=1
	cp ./espressif/bin/upgrade/user1.2048.new.3.bin ./staging/esp.factory.bin
	python3 ./tools/sign.py --prepend_size_word --rtip ./binaries/robot.axf --body ./binaries/syscon.axf ./staging/factory.safe
	make -C espressif clean

factory_upgrade: esp_factory
	cd espressif_factory_upgrade; python3 build.py
	python3 ./tools/sign.py $(SIGN_FLAGS) --wifi espressif_factory_upgrade/firmware/upgrader-image.bin --rtip ./binaries/robot.axf --body ./binaries/syscon.axf ./staging/factory_upgrade.safe
  
onBuildServer: esp

clean:
	make -C clad clean
	make -C espressif/app clean
	make -C espressif_bootloader clean
	make -C espressif_flasher clean
	make -C espressif_factory_upgrade clean
	rm -f $(VERSION_HEADER)
