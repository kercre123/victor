## @file Robot Firmware Makefile for Cozmo
# @author Daniel Casner
#
# Initially only handling Espressif firmware and pre/post-build steps for K02 and nrf51. May eventually have targets for
# all firmware.

ANKI_LOG_PP=tools/ankiLogPP.py
ANKI_LOG_TABLE_JSON=../resources/config/basestation/AnkiLogStringTables.json
ANKI_LOG_SRC_DIRS=supervisor/src k02_hal espressif/app sim_hal/ syscon/hal

UV4=C:/Keil_v5/UV4/UV4.exe
PY=python3

GENERATED_DIR=generated
BUILD_DIR=build

EXTERNALS_DIR=../EXTERNALS/firmware

DIFFIE_KEY=certs/diffie.pem
DEVELOPMENT_KEY=certs/development.pem
AES_KEY=FA67CE138A376CD21A26118615326AA5
SIGN_FLAGS=--sign $(AES_KEY) $(DEVELOPMENT_KEY)
PUBLIC_KEY_EXPORTER=tools/export_public.py
PUBLIC_KEYS_HEADER=$(GENERATED_DIR)/publickeys.h
CALCMONT_DEPS=tools/calcMontgomery.cpp ../crypto/bignum.cpp

WIFI_BINARY=$(BUILD_DIR)/esp.user.bin
RTIP_BINARY=$(BUILD_DIR)/robot.axf
BODY_BINARY=$(BUILD_DIR)/syscon.axf
MAP_FILES=$(BUILD_DIR)/esp.map $(BUILD_DIR)/robot.map $(BUILD_DIR)/syscon.map
OTA_FILE=releases/cozmo.safe

.PHONY: clean AnkiLog purgeAnkiLog verboseAnkiLog esp k02 nrf sim fresh app espBuild

########################################################################################################################
# Build rules for engine / app

sim: $(ANKI_LOG_TABLE_JSON)
	make fourzero -C clad -j4

app: $(ANKI_LOG_TABLE_JSON)

# Default to make engine (simulator build simple)
all: sim app esp k02 nrf

########################################################################################################################
# Convenience rules

dev: AnkiLog nrf k02 esp

ota: $(OTA_FILE) $(MAP_FILES)

clean:
	make -C clad clean
	make -C espressif/app clean
	make -C espressif_bootloader clean
	#rm -rf $(BUILD_DIR)
	rm -rf $(GENERATED_DIR)

fresh: clean $(ANKI_LOG_TABLE_JSON) all python
	rm -Rf build ./syscon/build

python:
	make $@ -C clad -j4

$(BUILD_DIR):
	mkdir -p $@

$(GENERATED_DIR):
	mkdir -p $@

nrf: $(PUBLIC_KEYS_HEADER) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

k02: $(ANKI_LOG_TABLE_JSON) $(ANKI_LOG_TABLE_JSON)
	make $@ -C clad -j4

k02_fix:
	$(PY) ./tools/boot_header.py ./build/app/robot.axf

esp: $(ANKI_LOG_TABLE_JSON) $(PUBLIC_KEYS_HEADER)
	make $@ -C clad -j4

pull:
	$(PY) pull_server_firmware.py

vesp:
	vagrant ssh -c "make -C /vagrant/robot esp"

########################################################################################################################
# Code generation and preprocessing

$(ANKI_LOG_TABLE_JSON): $(ANKI_LOG_PP) $(shell /usr/bin/find $(ANKI_LOG_SRC_DIRS) -type f)
	$(PY) $(ANKI_LOG_PP) --string-table --output $@ $(ANKI_LOG_SRC_DIRS)

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

########################################################################################################################
# Firmware Binaries

keil:
	$(UV4) -c ./cozmo.uvmpw
	$(UV4) -b ./cozmo.uvmpw

espBuild: esp AnkiLog
	make -C espressif lib/libmain_w.a
	make -C espressif/app ROBOT_INC_DIR=$$PWD/include BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3 LD_FILE=../ld/espressif_application.ld
	$(PY) ./tools/wifiAppImageHeader.py ./espressif/bin/upgrade/user1.2048.new.3.bin $(WIFI_BINARY)

$(OTA_FILE): Makefile $(WIFI_BINARY) $(RTIP_BINARY) $(BODY_BINARY) tools/sign.py python
	mkdir -p $(dir $@)
	$(PY) ./tools/sign.py -c $(BUILD_NUMBER) $(SIGN_FLAGS) --wifi $(WIFI_BINARY) --rtip $(RTIP_BINARY)  --body $(BODY_BINARY) $@

$(BUILD_DIR)/esp.map: espressif/app/esp.map
	cp $< $@

$(BUILD_DIR)/robot.map: $(BUILD_DIR)/app/robot.map
	cp $< $@

$(BUILD_DIR)/syscon.map: syscon/build/app/syscon.map
	cp $< $@

########################################################################################################################
# Build server steps

server.1: nrf k02
	tar -cjf generated.tar.bz2 $(GENERATED_DIR)

server.2: $(RTIP_BINARY) $(BODY_BINARY) # Run Keil cozmo.uvmpw batch build to generate these files
	tar -cjf keil.tar.bz2 $(RTIP_BINARY) $(BODY_BINARY) $(BUILD_DIR)/app/robot.map syscon/build/app/syscon.map

server.3: espBuild $(OTA_FILE) $(MAP_FILES)
	tar -czf firmware.tar.gz $(OTA_FILE) $(WIFI_BINARY) $(RTIP_BINARY) $(BODY_BINARY) $(MAP_FILES)
