## @file Robot Firmware Makefile for Cozmo
# @author Daniel Casner
#
# Initially only handling Espressif firmware and pre/post-build steps for K02 and nrf51. May eventually have targets for
# all firmware.

ANKI_LOG_PP=tools/ankiLogPP.py
ANKI_LOG_TABLE_JSON=../resources/config/basestation/AnkiLogStringTables.json
ANKI_LOG_SRC_DIRS=supervisor/src hal espressif/app sim_hal/ syscon/hal
ANKI_LOG_ST_ERR_MSG='Anki log pre-preprocessing incomplete. Run `make dev` to process macros. If you see this on the build server, you need to merge locally, run `make dev` and push again.'

UV4=C:/Keil_v5/UV4/UV4.exe
PY=python3
ON_ERR=tools/on_error.bash

GENERATED_DIR=generated
BUILD_DIR=build
RELEASE_DIR=release

EXTERNALS_DIR=../EXTERNALS/firmware

DIFFIE_KEY=certs/diffie.pem
DEVELOPMENT_KEY=certs/development.pem
AES_KEY=FA67CE138A376CD21A26118615326AA5
SIGN_FLAGS=--sign $(AES_KEY) $(DEVELOPMENT_KEY)
PUBLIC_KEY_EXPORTER=tools/export_public.py
PUBLIC_KEYS_HEADER=$(GENERATED_DIR)/publickeys.h
CALCMONT_DEPS=tools/calcMontgomery.cpp ../crypto/bignum.cpp
BUILD_TYPE_HEADER=$(GENERATED_DIR)/build_type.h

WIFI_BINARY=$(BUILD_DIR)/esp.user.bin
RTIP_BINARY=$(BUILD_DIR)/robot.axf
BODY_BINARY=$(BUILD_DIR)/syscon.axf
MAP_FILES=$(BUILD_DIR)/esp.map $(BUILD_DIR)/esp.S $(BUILD_DIR)/robot.map $(BUILD_DIR)/syscon.map
OTA_FILE=$(RELEASE_DIR)/cozmo.safe

BUILD_TYPE?=DEVELOPMENT

.PHONY: clean AnkiLog purgeAnkiLog verboseAnkiLog esp k02 nrf sim fresh app espBuild server.1 server.2 server.3 $(BUILD_TYPE_HEADER)

########################################################################################################################
# Build rules for engine / app

sim: $(ANKI_LOG_TABLE_JSON) $(BUILD_TYPE_HEADER)
	make fourzero -C clad -j4

app: $(ANKI_LOG_TABLE_JSON)

# Default to make engine (simulator build simple)
all: sim app esp k02 nrf

########################################################################################################################
# Convenience rules

dev: AnkiLog nrf k02 esp $(BUILD_TYPE_HEADER)

dev2: AnkiLog k02 $(BUILD_TYPE_HEADER)

ota: $(OTA_FILE)
	if test -e build/app/robot.map; then mv build/app/robot.map $(BUILD_DIR)/robot.map; fi
	if test -e syscon/build/app/syscon.map; then mv syscon/build/app/syscon.map $(BUILD_DIR)/syscon.map; fi

clean:
	make -C clad clean
	make -C espressif/app clean
	make -C espressif_bootloader clean
	rm -rf $(BUILD_DIR) $(GENERATED_DIR) $(RELEASE_DIR) syscon/build
	rm -rf generated.tar.bz2 keil.tar.bz2 firmware.tar.gz

fresh: clean $(ANKI_LOG_TABLE_JSON) all python
	rm -Rf build ./syscon/build

python:
	make $@ -C clad -j4

$(BUILD_DIR):
	mkdir -p $@

$(RELEASE_DIR):
	mkdir -p $@

$(GENERATED_DIR):
	mkdir -p $@

nrf: $(PUBLIC_KEYS_HEADER) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

k02: $(ANKI_LOG_TABLE_JSON) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

k02_fix:
	$(PY) ./tools/boot_header.py ./build/robot.axf

nrf_fix:
	$(PY) ./tools/boot_header.py ./build/syscon.axf


esp: $(ANKI_LOG_TABLE_JSON) $(PUBLIC_KEYS_HEADER)
	make $@ -C clad -j4

pull:
	$(PY) pull_server_firmware.py

vesp:
	vagrant ssh -c "make -C /vagrant/robot esp"

########################################################################################################################
# Code generation and preprocessing

$(ANKI_LOG_TABLE_JSON): $(ANKI_LOG_PP) $(shell /usr/bin/find $(ANKI_LOG_SRC_DIRS) -type f)
	$(ON_ERR) $(ANKI_LOG_ST_ERR_MSG) $(PY) $(ANKI_LOG_PP) --string-table --output $@ $(ANKI_LOG_SRC_DIRS)

AnkiLog: $(ANKI_LOG_PP) $(shell /usr/bin/find $(ANKI_LOG_SRC_DIRS) -type f)
	$(PY) $(ANKI_LOG_PP) --preprocessor $(ANKI_LOG_SRC_DIRS)

verboseAnkiLog: $(ANKI_LOG_PP)
	$(PY) $(ANKI_LOG_PP) -vvv --preprocessor --string-table --output $(ANKI_LOG_TABLE_JSON) $(ANKI_LOG_SRC_DIRS)

purgeAnkiLog: $(ANKI_LOG_PP)
	$(PY) $(ANKI_LOG_PP) --purge --preprocessor --string-table --output $(ANKI_LOG_TABLE_JSON) $(ANKI_LOG_SRC_DIRS)

$(PUBLIC_KEYS_HEADER): Makefile $(PUBLIC_KEY_EXPORTER) $(CALCMONT_DEPS) $(GENERATED_DIR) $(BUILD_DIR)
	$(PY) $(PUBLIC_KEY_EXPORTER) -d $(DIFFIE_KEY) -k $(DEVELOPMENT_KEY) -a $(AES_KEY) $@
	g++ -o build/calcMont $(CALCMONT_DEPS) -I ../crypto -I $(GENERATED_DIR)
	build/calcMont >> $@

$(BUILD_TYPE_HEADER): $(GENERATED_DIR)
	echo "#define " $(BUILD_TYPE) 1 > $@

########################################################################################################################
# Firmware Binaries

keil:
	$(UV4) -c ./cozmo.uvmpw
	$(UV4) -b ./cozmo.uvmpw

espBuild: esp AnkiLog
	make -C espressif lib/libmain_w.a
	make -C espressif/app ROBOT_INC_DIR=$$PWD/include ROBOT_HAL_INC_DIR=$$PWD/hal/include BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3 LD_FILE=../ld/espressif_application.ld
	$(PY) ./tools/wifiAppImageHeader.py ./espressif/bin/upgrade/user1.2048.new.3.bin $(WIFI_BINARY)
	cp espressif/app/esp.map $(BUILD_DIR)/esp.map
	cp ./espressif/bin/upgrade/user1.2048.new.3.S $(BUILD_DIR)/esp.S
	@echo "############################################"
	@echo "#Espressif firmware build with build type:"
	@cat $(BUILD_TYPE_HEADER)
	@echo "############################################"

esp_factory: python dev esp
  ifneq ($(BUILD_TYPE),FACTORY)
		@echo "must call with BUILD_TYPE=FACTORY"
  else
		make -C espressif clean
		cd espressif_bootloader; ./build.sh; cd ..
		make esp -C clad -j4
		make -C espressif/app ROBOT_INC_DIR=$$PWD/include ROBOT_HAL_INC_DIR=$$PWD/hal/include BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3 LD_FILE=../ld/espressif_factory.ld FACTORY_FIRMWARE=1
		test `wc -c ./espressif/bin/upgrade/user1.2048.new.3.bin | awk '{print $$1}'` -le 294912 # Make sure the firmware fits in segment
		cp ./espressif/bin/upgrade/user1.2048.new.3.bin $(BUILD_DIR)/esp.factory.bin
		python3 ./tools/sign.py --prepend_size_word --rtip $(BUILD_DIR)/robot.axf --body  $(BUILD_DIR)/syscon.axf $(BUILD_DIR)/factory.safe
		cp espressif/app/esp.map $(BUILD_DIR)/esp.factory.map
		cp ./espressif/bin/upgrade/user1.2048.new.3.S $(BUILD_DIR)/esp.factory.S
		@echo "############################################"
		@echo "#Espressif firmware build with build type:"
		@cat $(BUILD_TYPE_HEADER)
		@echo "############################################"
		make -C espressif clean
  endif

factory_upgrade: esp_factory $(RELEASE_DIR)
	cd espressif_factory_upgrade; python3 build.py
	python3 ./tools/sign.py $(SIGN_FLAGS) --wifi espressif_factory_upgrade/firmware/upgrader-image.bin --rtip ./build/robot.axf --body ./build/syscon.axf ./release/factory_upgrade.safe

$(OTA_FILE): Makefile $(RELEASE_DIR) $(WIFI_BINARY) $(RTIP_BINARY) $(BODY_BINARY) tools/sign.py python
	$(PY) ./tools/sign.py -c $(BUILD_NUMBER) -t $(BUILD_TYPE_HEADER) $(SIGN_FLAGS) --wifi $(WIFI_BINARY) --rtip $(RTIP_BINARY)  --body $(BODY_BINARY) $@

update_k02_bootloader.safe: 
	$(PY) ./tools/sign.py -c 9999 $(SIGN_FLAGS) --wifi $(WIFI_BINARY) --rtip ./build/recover/robot_recover.axf  --body $(BODY_BINARY) $@

$(BUILD_DIR)/syscon.map:
	if test -e syscon/build/app/syscon.map; then cp syscon/build/app/syscon.map $@; fi

########################################################################################################################
# Build server steps

server.1: nrf k02 $(BUILD_TYPE_HEADER)
	tar -cjf generated.tar.bz2 $(GENERATED_DIR)

server.2: $(RTIP_BINARY) $(BODY_BINARY) # Run Keil cozmo.uvmpw batch build to generate these files
	tar -cjf keil.tar.bz2 $(RTIP_BINARY) $(BODY_BINARY) $(BUILD_DIR)/app/robot.map syscon/build/app/syscon.map

server.3: $(BUILD_TYPE_HEADER) espBuild $(OTA_FILE)
	echo $(BUILD_NUMBER) > VERSION
	mv build/app/robot.map build/robot.map
	mv syscon/build/app/syscon.map build/syscon.map
	tar -czf firmware.tar.gz VERSION $(OTA_FILE) $(WIFI_BINARY) $(RTIP_BINARY) $(BODY_BINARY) $(MAP_FILES)
