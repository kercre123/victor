# CLAD makefile by Pauley
# Customized for Robot by Daniel

CLAD_BASE_DIR=../../victor-clad/tools/message-buffers
CLAD_EMITTER_DIR=$(CLAD_BASE_DIR)/emitters
CLAD_LIB_DIR=$(CLAD_BASE_DIR)/clad
CLAD_DEPENDENCIES=$(CLAD_LIB_DIR)/*.py
CLAD_CPP=$(CLAD_EMITTER_DIR)/CPP_emitter.py
CLAD_CPPLITE=$(CLAD_EMITTER_DIR)/CPPLite_emitter.py
CLAD_CSHARP=$(CLAD_EMITTER_DIR)/CSharp_emitter.py
CLAD_PYTHON=$(CLAD_EMITTER_DIR)/Python_emitter.py
CLAD_CPP_DECL=./cozmo_CPP_declarations_emitter.py
CLAD_CPP_SWITCH=./cozmo_CPPLite_switch_emitter.py
CLAD_CPPLITE_SEND_HELPER=./cozmo_CPPLite_send_helper_emitter.py
CLAD_CPPLITE_SEND_TO_ENGINE_HELPER=./cozmo_CPPLite_sendToEngine_helper_emitter.py
CLAD_CPPLITE_SEND_TO_ROBOT_HELPER=./cozmo_CPPLite_sendToRobot_helper_emitter.py
CLAD_HASH=$(CLAD_EMITTER_DIR)/ASTHash_emitter.py

INPUT_DIR=./src/
OUTPUT_DIR:=../generated/clad/
OUTPUT_DIR_PYTHON:=../generated/cladPython/
INPUT_FILES=$(shell cd $(INPUT_DIR); /usr/bin/find . -type f -iname '*.clad')
OUTPUT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, %.h, $(INPUT_FILES))
OUTPUT_H_AND_CPP=$(OUTPUT_H) $(OUTPUT_CPP)
OUTPUT_PYTHON=$(patsubst %.clad, %.py, $(INPUT_FILES))
ENGINE_TO_ROBOT_UNION:=./clad/robotInterface/messageEngineToRobot
ROBOT_TO_ENGINE_UNION:=./clad/robotInterface/messageRobotToEngine
BODY_START_TAG:=0x01
BODY_END_TAG:=0x2F
RTIP_START_TAG:=0x30
RTIP_END_TAG:=0x7F
WIFI_START_TAG:=0x80
WIFI_END_TAG:=0xAF

PYTHON=python3

vpath %.clad $(INPUT_DIR)
vpath %.h $(OUTPUT_DIR)/robot/
vpath %.cpp $(OUTPUT_DIR)/robot/
vpath %.def $(OUTPUT_DIR)/robot/
vpath %.py $(OUTPUT_DIR_PYTHON)/robot/

.PHONY: show clean

all: fourzero lightCube

cpp: $(OUTPUT_H_AND_CPP) $(ENGINE_TO_ROBOT_UNION)_hash.h $(ROBOT_TO_ENGINE_UNION)_hash.h

python: $(OUTPUT_PYTHON) $(ENGINE_TO_ROBOT_UNION)_hash.py $(ROBOT_TO_ENGINE_UNION)_hash.py

%_switch_group_anim.def : %.clad $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR)robot/$*_switch_group_anim.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_switch_group_anim.def; fi
	$(PYTHON) $(CLAD_CPP_SWITCH) --group=anim -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_switch_group_anim.def

%_switch.def : %.clad $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR)robot/$*_switch.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_switch.def; fi
	$(PYTHON) $(CLAD_CPP_SWITCH) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_switch.def

%_switch_from_$(BODY_START_TAG)_to_$(BODY_END_TAG).def : %.clad $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR)robot/$*_switch_from_$(BODY_START_TAG)_to_$(BODY_END_TAG).def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_switch_from_$(BODY_START_TAG)_to_$(BODY_END_TAG).def; fi
	$(PYTHON) $(CLAD_CPP_SWITCH) --start=$(BODY_START_TAG) --end=$(BODY_END_TAG) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_switch_from_$(BODY_START_TAG)_to_$(BODY_END_TAG).def

%_switch_from_$(RTIP_START_TAG)_to_$(RTIP_END_TAG).def : %.clad $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR)robot/$*_switch_from_$(RTIP_START_TAG)_to_$(RTIP_END_TAG).def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_switch_from_$(RTIP_START_TAG)_to_$(RTIP_END_TAG).def; fi
	$(PYTHON) $(CLAD_CPP_SWITCH) --start=$(RTIP_START_TAG) --end=$(RTIP_END_TAG) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_switch_from_$(RTIP_START_TAG)_to_$(RTIP_END_TAG).def

%_switch_from_$(WIFI_START_TAG)_to_$(WIFI_END_TAG)_group_anim.def : %.clad $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR)robot/$*_switch_from_$(WIFI_START_TAG)_to_$(WIFI_END_TAG)_group_anim.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_switch_from_$(WIFI_START_TAG)_to_$(WIFI_END_TAG)_group_anim.def; fi
	$(PYTHON) $(CLAD_CPP_SWITCH) --start=$(WIFI_START_TAG) --end=$(WIFI_END_TAG) --group=anim -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_switch_from_$(WIFI_START_TAG)_to_$(WIFI_END_TAG)_group_anim.def

%_declarations.def : %.clad $(CLAD_CPP_DECL)
	if test -f $(OUTPUT_DIR)robot/$*_declarations.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_declarations.def; fi
	$(PYTHON) $(CLAD_CPP_DECL)   -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_declarations.def

%_send_helper.h : %.clad $(CLAD_CPPLITE_SEND_HELPER)
	if test -f $(OUTPUT_DIR)robot/$*_send_helper.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*_send_helper.h; fi
	$(PYTHON) $(CLAD_CPPLITE_SEND_HELPER) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_send_helper.h

%_sendToEngine_helper.h : %.clad $(CLAD_CPPLITE_SEND_TO_ENGINE_HELPER)
	if test -f $(OUTPUT_DIR)robot/$*_sendToEngine_helper.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*_sendToEngine_helper.h; fi
	$(PYTHON) $(CLAD_CPPLITE_SEND_TO_ENGINE_HELPER) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_sendToEngine_helper.h

%_sendToRobot_helper.h : %.clad $(CLAD_CPPLITE_SEND_TO_ROBOT_HELPER)
	if test -f $(OUTPUT_DIR)robot/$*_sendToRobot_helper.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*_sendToRobot_helper.h; fi
	$(PYTHON) $(CLAD_CPPLITE_SEND_TO_ROBOT_HELPER) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_sendToRobot_helper.h

%.h %.cpp : %.clad $(CLAD_DEPENDENCIES) $(CLAD_CPPLITE)
	if test -f $(OUTPUT_DIR)robot/$*.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*.h; fi
	if test -f $(OUTPUT_DIR)robot/$*.cpp; then chmod -f 777 $(OUTPUT_DIR)robot/$*.cpp; fi
	$(PYTHON) $(CLAD_CPPLITE) --max-message-size 1400 -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*.h
	chmod -f 555 $(OUTPUT_DIR)robot/$*.cpp

%_hash.h : %.clad $(CLAD_DEPENDENCIES) $(CLAD_HASH) $(shell /usr/bin/find $(INPUT_DIR) -type f)
	if test -f $(OUTPUT_DIR)robot/$@; then chmod -f 777 $(OUTPUT_DIR)robot/$@; fi
	$(PYTHON) $(CLAD_HASH) -I $(INPUT_DIR) -o $(OUTPUT_DIR)robot/$@ $<
	chmod -f 555 $(OUTPUT_DIR)robot/$@

%_hash.py: %.clad $(CLAD_DEPENDENCIES) $(CLAD_HASH) $(shell /usr/bin/find $(INPUT_DIR) -type f)
	if test -f $(OUTPUT_DIR_PYTHON)robot/$@; then chmod -f 777 $(OUTPUT_DIR_PYTHON)robot/$@; fi
	$(PYTHON) $(CLAD_HASH) -I $(INPUT_DIR) -o $(OUTPUT_DIR_PYTHON)robot/$@ $<
	chmod -f 555 $(OUTPUT_DIR_PYTHON)robot/$@

%.py : %.clad $(CLAD_DEPENDENCIES) $(CLAD_PYTHON)
	if test -f $(OUTPUT_DIR_PYTHON)robot/$*.py; then chmod -f 777 $(OUTPUT_DIR_PYTHON)robot/$*.py; fi
	$(PYTHON) $(CLAD_PYTHON) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR_PYTHON)robot $*.clad
	chmod -f 555 $(OUTPUT_DIR_PYTHON)robot/$*.py

cpplilib: cpp
	@rm -f $(OUTPUT_DIR)/clad.cpp
	cd $(OUTPUT_DIR); /usr/bin/find . -type f -iname '*.cpp' > cpps.list; awk '{ print "#include \"" $$0 "\"" }' <cpps.list >clad.cpp
	@rm $(OUTPUT_DIR)cpps.list

hashes:
	$(PYTHON) $(CLAD_HASH) -C $(INPUT_DIR) $(ENGINE_TO_ROBOT_UNION).clad
	$(PYTHON) $(CLAD_HASH) -C $(INPUT_DIR) $(ROBOT_TO_ENGINE_UNION).clad

nrf: cpplilib $(ENGINE_TO_ROBOT_UNION)_switch_from_$(BODY_START_TAG)_to_$(BODY_END_TAG).def $(ENGINE_TO_ROBOT_UNION)_declarations.def $(ROBOT_TO_ENGINE_UNION)_send_helper.h $(ENGINE_TO_ROBOT_UNION)_send_helper.h

# sendToEngine and sendToRobot helpers are Victor-specific
k02: cpplilib $(ENGINE_TO_ROBOT_UNION)_switch_from_$(RTIP_START_TAG)_to_$(RTIP_END_TAG).def $(ENGINE_TO_ROBOT_UNION)_declarations.def $(ROBOT_TO_ENGINE_UNION)_send_helper.h $(ENGINE_TO_ROBOT_UNION)_send_helper.h $(ROBOT_TO_ENGINE_UNION)_sendToEngine_helper.h $(ENGINE_TO_ROBOT_UNION)_sendToRobot_helper.h

esp: cpp $(ENGINE_TO_ROBOT_UNION)_switch_from_$(WIFI_START_TAG)_to_$(WIFI_END_TAG)_group_anim.def $(ENGINE_TO_ROBOT_UNION)_declarations.def $(ROBOT_TO_ENGINE_UNION)_send_helper.h $(ENGINE_TO_ROBOT_UNION)_send_helper.h

lightCube: $(LIGHT_CUBE_UNION)_declarations.def $(LIGHT_CUBE_UNION)_send_helper.h $(LIGHT_CUBE_UNION)_switch.def

fourzero: cpp $(ENGINE_TO_ROBOT_UNION)_switch_group_anim.def $(ENGINE_TO_ROBOT_UNION)_declarations.def $(ROBOT_TO_ENGINE_UNION)_send_helper.h $(ENGINE_TO_ROBOT_UNION)_send_helper.h

show:
	echo "*** input files ***"
	echo $(INPUT_FILES)
	echo "*** output C++ files ***"
	echo $(OUTPUT_H_AND_CPP)
	echo "*** output python files ***"
	echo $(OUTPUT_PYTHON)

clean:
	if test -d $(OUTPUT_DIR); then chmod -Rf 777 $(OUTPUT_DIR); fi
	if test -d $(OUTPUT_DIR_PYTHON); then chmod -Rf 777 $(OUTPUT_DIR_PYTHON); fi
	rm -rf $(OUTPUT_DIR) $(OUTPUT_DIR_PYTHON)
